/*
* PFTS - Polymer Field Theory Simulator
*
* Copyright 2013, David Morse (morse012@.umn.edu)
* Distributed under the terms of the GNU General Public License.
*/

#include "PolymerDescriptor.h"

namespace Pfts{ 

   /*
   * Constructor.
   */
   PolymerDescriptor::PolymerDescriptor()
    : blocks_(),
      vertices_(),
      solverIds_(),
      nBlock_(0),
      nVertex_(0),
      nSolver_(0),
      volume_(0.0)
   {
      setClassName("PolymerDescriptor");
   }

   void PolymerDescriptor::readParameters(std::istream& in)
   {
      read<int>(in, "nBlock", nBlock_);
      read<int>(in, "nVertex", nVertex_);

      // Allocate all arrays
      blocks_.allocate(nBlock_);
      vertices_.allocate(nVertex_);
      solverIds_.allocate(2*nBlock_);

      readDArray<Block>(in, "blocks", blocks_, nBlock_);

      // Allocate array of vertices and set vertex indices
      for (int vertexId = 0; vertexId < nVertex_; ++vertexId) {
         vertices_[vertexId].setId(vertexId);
      }

      // Add blocks to vertices
      int vertexId0, vertexId1;
      Block* blockPtr;
      for (int blockId = 0; blockId < nBlock_; ++blockId) {
          blockPtr = &(blocks_[blockId]);
          vertexId0 = blockPtr->vertexId(0);
          vertexId1 = blockPtr->vertexId(1);
          vertices_[vertexId0].addBlock(*blockPtr);
          vertices_[vertexId1].addBlock(*blockPtr);
      }

      makePlan();
   }

   void PolymerDescriptor::makePlan()
   {
      if (nSolver_ != 0) {
         UTIL_THROW("nSolver !=0 on entry");
      }

      // Allocate and initialize isFinished matrix
      DMatrix<bool> isFinished;
      isFinished.allocate(nBlock_, 2);
      for (int iBlock = 0; iBlock < nBlock_; ++iBlock) {
         for (int iDirection = 0; iDirection < 2; ++iDirection) {
            isFinished(iBlock, iDirection) = false;
         }
      }

      Pair<int> solverId;
      Vertex* inVertexPtr = 0;
      int inVertexId = -1;
      bool isReady;
      while (nSolver_ < nBlock_*2) {
         for (int iBlock = 0; iBlock < nBlock_; ++iBlock) {
            for (int iDirection = 0; iDirection < 2; ++iDirection) {
               if (isFinished(iBlock, iDirection) == false) {
                  inVertexId = blocks_[iBlock].vertexId(iDirection);
                  inVertexPtr = &vertices_[inVertexId];
                  isReady = true;
                  for (int j = 0; j < inVertexPtr->size(); ++j) {
                     solverId = inVertexPtr->inSolverId(j);
                     if (solverId[0] != iBlock) {
                        if (!isFinished(solverId[0], solverId[1])) {
                           isReady = false;
                           break;
                        }
                     }
                  }
                  if (isReady) {
                     solverIds_[nSolver_][0] = iBlock;
                     solverIds_[nSolver_][1] = iDirection;
                     isFinished(iBlock, iDirection) = true;
                     ++nSolver_;
                  }
               }
            }
         }
      }

   }

} 
