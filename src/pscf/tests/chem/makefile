BLD_DIR_REL =../../..
include $(BLD_DIR_REL)/config.mk
include $(BLD_DIR)/util/config.mk
include $(BLD_DIR)/pscf/config.mk
include $(SRC_DIR)/pscf/patterns.mk
include $(SRC_DIR)/util/sources.mk
include $(SRC_DIR)/pscf/sources.mk
include $(SRC_DIR)/pscf/tests/chem/sources.mk

TEST=pscf/tests/chem/Test

all: $(pscf_tests_chem_OBJS) $(BLD_DIR)/$(TEST)

includes:
	echo $(INCLUDES)

run: $(pscf_tests_chem_OBJS) $(BLD_DIR)/$(TEST)
	$(BLD_DIR)/$(TEST) $(SRC_DIR)/pscf/tests/chem/ > log
	@echo `grep failed log` ", "\
              `grep successful log` "in pscf/tests/log" > count
	@cat count

clean:
	rm -f $(pscf_tests_chem_OBJS) $(pscf_tests_chem_OBJS:.o=.d)
	rm -f $(BLD_DIR)/$(TEST) $(BLD_DIR)/$(TEST).d
	rm -f log count 

-include $(pscf_tests_chem_OBJS:.o=.d)
-include $(pscf_tests_chem_OBJS:.o=.d)
