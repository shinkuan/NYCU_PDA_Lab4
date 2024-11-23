# Target name
TARGET = D2DGRter

# Folder paths
SRCDIR = src
OBJDIR = obj
INCDIR = inc

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++14 -fopenmp -I$(INCDIR) -Wall -Wextra -Wpedantic -Werror -g -Og

# Source and object files
SOURCES := $(wildcard $(SRCDIR)/*.cpp) main.cpp
OBJECTS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(notdir $(SOURCES)))

# Default target
all: $(TARGET)

# Compile target
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)

# Compile source files into object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/main.o: main.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create obj directory if it doesn't exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Clean up object files and executable
clean:
	rm -rf $(OBJDIR)/*.o $(TARGET)

# Run the compiled executable
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run