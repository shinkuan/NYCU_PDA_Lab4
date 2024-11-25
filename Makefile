# Target name
TARGET = D2DGRter
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
    RM = del /Q /F
    MKDIR = if not exist "$(OBJDIR)" mkdir
    PATH_SEP = \\
else
    RM = rm -rf
    MKDIR = mkdir -p
    PATH_SEP = /
endif

# Folder paths
SRCDIR = src
OBJDIR = obj
INCDIR = inc

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++14 -fopenmp -I$(INCDIR) -Wall -Wextra -Wpedantic
RELEASE_FLAGS = -O3
DEBUG_FLAGS = -g -Og -DDEBUG

# Source and object files
SOURCES := $(wildcard $(SRCDIR)/*.cpp) main.cpp
OBJECTS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(notdir $(SOURCES)))

# Default target (release build)
all: CXXFLAGS += $(RELEASE_FLAGS)
all: $(TARGET)

# Compile target
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)

# Compile source files into object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/main.o: main.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Debug target
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)

# Create obj directory if it doesn't exist
$(OBJDIR):
	$(MKDIR) $(OBJDIR)

# Clean up object files and executable
clean:
ifeq ($(OS),Windows_NT)
	@if exist "$(OBJDIR)\*.o" $(RM) "$(OBJDIR)\*.o"
	@if exist "$(TARGET)" $(RM) "$(TARGET)"
else
	$(RM) $(OBJDIR)/*.o $(TARGET)
endif

# Run the compiled executable
run: $(TARGET)
ifeq ($(OS),Windows_NT)
	.\$(TARGET)
else
	./$(TARGET)
endif

.PHONY: all clean run debug