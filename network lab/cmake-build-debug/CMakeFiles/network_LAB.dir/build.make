# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.13

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake

# The command to remove a file.
RM = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/xiayutian/CLionProjects/network_LAB

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/xiayutian/CLionProjects/network_LAB/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/network_LAB.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/network_LAB.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/network_LAB.dir/flags.make

CMakeFiles/network_LAB.dir/main.cpp.o: CMakeFiles/network_LAB.dir/flags.make
CMakeFiles/network_LAB.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/xiayutian/CLionProjects/network_LAB/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/network_LAB.dir/main.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/network_LAB.dir/main.cpp.o -c /Users/xiayutian/CLionProjects/network_LAB/main.cpp

CMakeFiles/network_LAB.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/network_LAB.dir/main.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/xiayutian/CLionProjects/network_LAB/main.cpp > CMakeFiles/network_LAB.dir/main.cpp.i

CMakeFiles/network_LAB.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/network_LAB.dir/main.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/xiayutian/CLionProjects/network_LAB/main.cpp -o CMakeFiles/network_LAB.dir/main.cpp.s

# Object files for target network_LAB
network_LAB_OBJECTS = \
"CMakeFiles/network_LAB.dir/main.cpp.o"

# External object files for target network_LAB
network_LAB_EXTERNAL_OBJECTS =

network_LAB: CMakeFiles/network_LAB.dir/main.cpp.o
network_LAB: CMakeFiles/network_LAB.dir/build.make
network_LAB: CMakeFiles/network_LAB.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/xiayutian/CLionProjects/network_LAB/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable network_LAB"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/network_LAB.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/network_LAB.dir/build: network_LAB

.PHONY : CMakeFiles/network_LAB.dir/build

CMakeFiles/network_LAB.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/network_LAB.dir/cmake_clean.cmake
.PHONY : CMakeFiles/network_LAB.dir/clean

CMakeFiles/network_LAB.dir/depend:
	cd /Users/xiayutian/CLionProjects/network_LAB/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/xiayutian/CLionProjects/network_LAB /Users/xiayutian/CLionProjects/network_LAB /Users/xiayutian/CLionProjects/network_LAB/cmake-build-debug /Users/xiayutian/CLionProjects/network_LAB/cmake-build-debug /Users/xiayutian/CLionProjects/network_LAB/cmake-build-debug/CMakeFiles/network_LAB.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/network_LAB.dir/depend
