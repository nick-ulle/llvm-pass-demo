# Description:
#		Build multiple, distinct binaries that use the LLVM C++ API.

# Configuration
# -------------
VPATH 	  := src
BUILD_DIR ?= build
BIN_DIR   ?= bin

LLVM_CONFIG ?= llvm-config

COMMON_FLAGS := -Wextra
CPPFLAGS     += $(shell $(LLVM_CONFIG) --cppflags) -I$(SRC_DIR)
CXXFLAGS		 += $(COMMON_FLAGS) $(shell $(LLVM_CONFIG) --cxxflags)
LDFLAGS			 += $(shell $(LLVM_CONFIG) --ldflags)


# Targets
# -------
.PHONY : all
all : $(addprefix $(BIN_DIR)/,pass_demo)

pass_demo := pass.cpp

$(BIN_DIR)/pass_demo : $(pass_demo:%.cpp=$(BUILD_DIR)/%.o) | $(BIN_DIR)
	$(call linker,irreader)


# Generic Rules & Recipes
# -----------------------
$(BUILD_DIR) $(BIN_DIR) :
	@mkdir $@

$(BUILD_DIR)/%.o : %.cpp | $(BUILD_DIR)
	@echo Compiling $< ...
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<
	@echo

# Canned recipe for linking. The LLVM libs in $(1) are passed to llvm-config.
define linker =
@echo Linking $@ ...
$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ \
	$(shell $(LLVM_CONFIG) --libs $(1) --system-libs)
@echo
endef

.PHONY : clean
clean :
	@echo Cleaning ...
	rm -r $(BUILD_DIR) $(BIN_DIR)
