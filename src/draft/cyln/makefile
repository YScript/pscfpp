#-----------------------------------------------------------------------
# The copy of this namespace-level makefile in the src/ directory is 
# copied to the bld/serial and bld/parallel directories by the setup
# script to create the copies in those directories. Only the copy in
# the src/ directory is stored in the repository.
#-----------------------------------------------------------------------
# Include makefiles

SRC_DIR_REL =..
include $(SRC_DIR_REL)/config.mk
include $(BLD_DIR)/util/config.mk
include $(BLD_DIR)/pscf/config.mk
include $(BLD_DIR)/cyln/config.mk
include $(SRC_DIR)/cyln/patterns.mk
include $(SRC_DIR)/util/sources.mk
include $(SRC_DIR)/pscf/sources.mk
include $(SRC_DIR)/cyln/sources.mk

#-----------------------------------------------------------------------
# Variable definition

PSCF_CL=$(BLD_DIR)/cyln/pscf_cl
#-----------------------------------------------------------------------
# Main targets 

all: $(cyln_OBJS) $(cyln_LIB) $(PSCF_CL_EXE)

clean:
	rm -f $(cyln_OBJS) $(cyln_OBJS:.o=.d)
	rm -f $(PSCF_CL).o $(PSCF_CL).d
	rm -f $(cyln_LIB)
	cd tests; $(MAKE) clean

veryclean:
	$(MAKE) clean
	-rm -f *.o 
	-rm -f *.d 
	-rm -f lib*.a


# Executable target

$(PSCF_CL_EXE): $(PSCF_CL).o $(PSCF_LIBS)
	$(CXX) $(LDFLAGS) -o $(PSCF_CL_EXE) $(PSCF_CL).o $(LIBS) 

# Short name for executable target (for convenience)
pscf_cl:
	$(MAKE) $(PSCF_CL_EXE)

#-----------------------------------------------------------------------
# Include dependency files

-include $(cyln_OBJS:.o=.d)
-include $(pscf_OBJS:.o=.d)
-include $(util_OBJS:.o=.d)
-include $(PSCF_CL).d 
