BLD_DIR_REL =../../..
include $(BLD_DIR_REL)/config.mk
include $(SRC_DIR)/pspc/include.mk
include $(SRC_DIR)/pspc/tests/system/sources.mk

TEST=pspc/tests/system/Test

all: $(pspc_tests_system_OBJS) $(BLD_DIR)/$(TEST)

includes:
	@echo $(INCLUDES)

libs:
	@echo $(LIBS)

run: $(pspc_tests_system_OBJS) $(BLD_DIR)/$(TEST)
	$(BLD_DIR)/$(TEST) $(SRC_DIR)/pspc/tests/ > log
	@echo `grep failed log` ", "\
              `grep successful log` "in pspc/tests/system/log" > count
	@cat count
clean:
	rm -f $(pspc_tests_system_OBJS) $(pspc_tests_system_OBJS:.o=.d)
	rm -f $(BLD_DIR)/$(TEST) $(BLD_DIR)/$(TEST).d
	rm -f log count 
	rm -f out/stars_hex out/*.log
	rm -f out/omega/*/*

-include $(pspc_tests_system_OBJS:.o=.d)
-include $(pspc_tests_system_OBJS:.o=.d)
