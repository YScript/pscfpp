#ifndef PSSP_GPU_PROPAGATOR_TPP
#define PSSP_GPU_PROPAGATOR_TPP

/*
* PSCF - Polymer Self-Consistent Field Theory
*
* Copyright 2016, The Regents of the University of Minnesota
* Distributed under the terms of the GNU General Public License.
*/

#include "Propagator.h"
#include "Block.h"
#include <thrust/reduce.h>
#include "device_launch_parameters.h"
#include <cuda.h>
#include <device_functions.h>
#include <thrust/count.h>
#include <pssp_gpu/GpuResources.h>
#include <pscf/mesh/Mesh.h>
//#include <Windows.h>

template<unsigned int blockSize>
__global__ void deviceInnerProduct(cufftReal* c, const cufftReal* a,
	const cufftReal* b, int size) {
        //int nThreads = blockDim.x * gridDim.x;
	int startID = blockIdx.x * blockDim.x + threadIdx.x;

	//do all pointwise multiplication
	volatile extern __shared__ cufftReal cache[];
	cufftReal temp = 0;
	//no need for loop here will be wrong.
	//for (int i = startID; i < size; i += nThreads) {
	temp += a[startID] * b[startID];
	//}
	cache[threadIdx.x] = temp;

	__syncthreads();

	if(blockSize >= 512) {
	  if (threadIdx.x < 256){
	    cache[threadIdx.x] += cache[threadIdx.x + 256];
	  }
	  __syncthreads();
	}
	if(blockSize >= 256) {
	  if (threadIdx.x < 128){
	    cache[threadIdx.x] += cache[threadIdx.x + 128];
	  }
	  __syncthreads();
	}
	if(blockSize >= 128) {
	  if (threadIdx.x < 64){
	    cache[threadIdx.x] += cache[threadIdx.x + 64];
	  }
	  __syncthreads();
	}
	//reduce operation
	//256/2 -- needs to be power of two
	//for (int j = blockDim.x / 2; j > 32; j /= 2) {
	//	if (threadIdx.x < j) {
	//		cache[threadIdx.x] += cache[threadIdx.x + j];
	//	}
	//	__syncthreads();
	//}
	

	if (threadIdx.x < 32) {
	  if(blockSize >= 64) cache[threadIdx.x] += cache[threadIdx.x + 32];
	  if(blockSize >= 32) cache[threadIdx.x] += cache[threadIdx.x + 16];
	  if(blockSize >= 16) cache[threadIdx.x] += cache[threadIdx.x + 8];
	  if(blockSize >= 8) cache[threadIdx.x] += cache[threadIdx.x + 4];
	  if(blockSize >= 4) cache[threadIdx.x] += cache[threadIdx.x + 2];
	  if(blockSize >= 2) cache[threadIdx.x] += cache[threadIdx.x + 1];

	}

	if (threadIdx.x == 0) {
		c[blockIdx.x] = cache[0];
	}
}

static __global__ void assignUniformReal(cufftReal* result, cufftReal uniform, int size) {
   int nThreads = blockDim.x * gridDim.x;
   int startID = blockIdx.x * blockDim.x + threadIdx.x;
   for(int i = startID; i < size; i += nThreads) {
      result[i] = uniform;
   }
}

static __global__ void assignReal(cufftReal* result, const cufftReal* rhs, int size) {
   int nThreads = blockDim.x * gridDim.x;
   int startID = blockIdx.x * blockDim.x + threadIdx.x;
   for(int i = startID; i < size; i += nThreads) {
      result[i] = rhs[i];
   }
}

static __global__ void inPlacePointwiseMul(cufftReal* a, const cufftReal* b, int size) {
   int nThreads = blockDim.x * gridDim.x;
   int startID = blockIdx.x * blockDim.x + threadIdx.x;
   for(int i = startID; i < size; i += nThreads) {
      a[i] *= b[i];
   }
}



namespace Pscf {
namespace Pssp_gpu {

   using namespace Util;

   /*
   * Constructor.
   */
   template <int D>
   Propagator<D>::Propagator()
    : blockPtr_(0),
      meshPtr_(0),
      ns_(0),
      isAllocated_(false)
   {
	   cudaMalloc((void**)&d_temp_, NUMBER_OF_BLOCKS * sizeof(cufftReal));
	   temp_ = new cufftReal[NUMBER_OF_BLOCKS];
   }

   /*
   * Destructor.
   */
   template <int D>
   Propagator<D>::~Propagator()
   {
	   delete[] temp_;
	   cudaFree(d_temp_);
   }

   template <int D>
   void Propagator<D>::allocate(int ns, const Mesh<D>& mesh)
   {
      ns_ = ns;
      meshPtr_ = &mesh;

      qFields_.allocate(ns);
      for (int i = 0; i < ns; ++i) {
         qFields_[i].allocate(mesh.dimensions());
      }
      isAllocated_ = true;
   }

   /*
   * Compute initial head QField from final tail QFields of sources.
   */
   template <int D>
   void Propagator<D>::computeHead()
   {

      // Reference to head of this propagator
      QField& qh = qFields_[0];

      // Initialize qh field to 1.0 at all grid points
      int nx = meshPtr_->size();

      //qh[ix] = 1.0;
      assignUniformReal<<<NUMBER_OF_BLOCKS, THREADS_PER_BLOCK>>>(qh.cDField(), 1.0, nx);
      

      // Pointwise multiply tail QFields of all sources
      // this could be slow with many sources. Should launch 1 kernel for the whole
      // function of computeHead
      for (int is = 0; is < nSource(); ++is) {
         if (!source(is).isSolved()) {
            UTIL_THROW("Source not solved in computeHead");
         }
         QField const& qt = source(is).tail();

         //qh[ix] *= qt[ix];
         inPlacePointwiseMul<<<NUMBER_OF_BLOCKS, THREADS_PER_BLOCK>>>(qh.cDField(), qt.cDField(), nx);
      }
   }

   /*
   * Solve the modified diffusion equation for this block.
   */
   template <int D>
   void Propagator<D>::solve()
   {
      UTIL_CHECK(isAllocated());
      computeHead();
      // Setup solver and solve
      block().setupFFT();
	  //cufftReal* qf;
	  //qf = new cufftReal;

      for (int iStep = 0; iStep < ns_ - 1; ++iStep) {
         block().step(qFields_[iStep], qFields_[iStep + 1]);
      }
	  //delete qf;
      setIsSolved(true);
   }

   /*
   * Solve the modified diffusion equation with specified initial field.
   */
   template <int D>
   void Propagator<D>::solve(QField const & head)
   {
      int nx = meshPtr_->size();
      UTIL_CHECK(head.capacity() == nx);

      // Initialize initial (head) field
      QField& qh = qFields_[0];
      // qh[i] = head[i];
      assignReal<<<NUMBER_OF_BLOCKS, THREADS_PER_BLOCK>>>(qh.cDField(), head.cDField(), nx);

      // Setup solver and solve
      for (int iStep = 0; iStep < ns_ - 1; ++iStep) {
         block().step(qFields_[iStep], qFields_[iStep + 1]);
      }
      setIsSolved(true);
   }

   /*
   * Integrate to calculate monomer concentration for this block
   */
   template <int D>
   double Propagator<D>::computeQ()
   {
      // Preconditions
      if (!isSolved()) {
         UTIL_THROW("Propagator is not solved.");
      }
      if (!hasPartner()) {
         UTIL_THROW("Propagator has no partner set.");
      }
      if (!partner().isSolved()) {
         UTIL_THROW("Partner propagator is not solved");
      }
      QField const& qh = head();
      QField const& qt = partner().tail();
      int nx = meshPtr_->size();
      UTIL_CHECK(qt.capacity() == nx);
      UTIL_CHECK(qh.capacity() == nx);

      // Take inner product of head and partner tail fields
      // cannot reduce assuming one propagator, qh == 1
      // polymers are divided into blocks midway through
      double Q = 0; 
   
	  //double PCFreq = 0.0;
	  //__int64 CounterStart = 0;
	  //LARGE_INTEGER li;
	  //QueryPerformanceFrequency(&li);
	  //PCFreq = double(li.QuadPart) / 1000.0;
	  //QueryPerformanceCounter(&li);
	  //CounterStart = li.QuadPart;

      Q = innerProduct(qh, qt, nx);
      Q /= double(nx);

	  //QueryPerformanceCounter(&li);
      return Q;
   }

   template <int D>
   cufftReal Propagator<D>::innerProduct(const QField& a, const QField& b, int size) {
	   
     switch(THREADS_PER_BLOCK){
     case 512:
       deviceInnerProduct<512><<<NUMBER_OF_BLOCKS, THREADS_PER_BLOCK, THREADS_PER_BLOCK * sizeof(cufftReal)>>>(d_temp_, a.cDField(), b.cDField(), size);
       break;
     case 256:
       deviceInnerProduct<256><<<NUMBER_OF_BLOCKS, THREADS_PER_BLOCK, THREADS_PER_BLOCK * sizeof(cufftReal)>>>(d_temp_, a.cDField(), b.cDField(), size);
       break;
     case 128:
       deviceInnerProduct<128><<<NUMBER_OF_BLOCKS, THREADS_PER_BLOCK, THREADS_PER_BLOCK * sizeof(cufftReal)>>>(d_temp_, a.cDField(), b.cDField(), size);
       break;
     case 64:
       deviceInnerProduct<64><<<NUMBER_OF_BLOCKS, THREADS_PER_BLOCK, THREADS_PER_BLOCK * sizeof(cufftReal)>>>(d_temp_, a.cDField(), b.cDField(), size);
       break;
     case 32:
       deviceInnerProduct<32><<<NUMBER_OF_BLOCKS, THREADS_PER_BLOCK, THREADS_PER_BLOCK * sizeof(cufftReal)>>>(d_temp_, a.cDField(), b.cDField(), size);
       break;
     case 16:
       deviceInnerProduct<16><<<NUMBER_OF_BLOCKS, THREADS_PER_BLOCK, THREADS_PER_BLOCK * sizeof(cufftReal)>>>(d_temp_, a.cDField(), b.cDField(), size);
       break;
     case 8:
       deviceInnerProduct<8><<<NUMBER_OF_BLOCKS, THREADS_PER_BLOCK, THREADS_PER_BLOCK * sizeof(cufftReal)>>>(d_temp_, a.cDField(), b.cDField(), size);
       break;
     case 4:
       deviceInnerProduct<4><<<NUMBER_OF_BLOCKS, THREADS_PER_BLOCK, THREADS_PER_BLOCK * sizeof(cufftReal)>>>(d_temp_, a.cDField(), b.cDField(), size);
       break;
     case 2:
       deviceInnerProduct<2><<<NUMBER_OF_BLOCKS, THREADS_PER_BLOCK, THREADS_PER_BLOCK * sizeof(cufftReal)>>>(d_temp_, a.cDField(), b.cDField(), size);
       break;
     case 1:
       deviceInnerProduct<1><<<NUMBER_OF_BLOCKS, THREADS_PER_BLOCK, THREADS_PER_BLOCK * sizeof(cufftReal)>>>(d_temp_, a.cDField(), b.cDField(), size);
       break;
     }
     
	   cudaMemcpy(temp_, d_temp_, NUMBER_OF_BLOCKS * sizeof(cufftReal), cudaMemcpyDeviceToHost);
	   cufftReal final = 0;
	   cufftReal c = 0;
	   //use kahan summation
	   for(int i = 0; i < NUMBER_OF_BLOCKS; ++i) {
		   cufftReal y = temp_[i] - c;
		   cufftReal t = final + y;
		   c = (t - final) - y;
		  final = t;
	   }
	   
	   return final;
   }

}
}





#endif