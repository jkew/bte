# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.18

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.18.4/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.18.4/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/jkew/dev/bte

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/jkew/dev/bte

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/local/Cellar/cmake/3.18.4/bin/cmake --regenerate-during-build -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache

.PHONY : rebuild_cache/fast

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake cache editor..."
	/usr/local/Cellar/cmake/3.18.4/bin/ccmake -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache

.PHONY : edit_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /Users/jkew/dev/bte/CMakeFiles /Users/jkew/dev/bte//CMakeFiles/progress.marks
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /Users/jkew/dev/bte/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean

.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named bte

# Build rule for target.
bte: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 bte
.PHONY : bte

# fast build rule for target.
bte/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/bte.dir/build.make CMakeFiles/bte.dir/build
.PHONY : bte/fast

bte.o: bte.cpp.o

.PHONY : bte.o

# target to build an object file
bte.cpp.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/bte.dir/build.make CMakeFiles/bte.dir/bte.cpp.o
.PHONY : bte.cpp.o

bte.i: bte.cpp.i

.PHONY : bte.i

# target to preprocess a source file
bte.cpp.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/bte.dir/build.make CMakeFiles/bte.dir/bte.cpp.i
.PHONY : bte.cpp.i

bte.s: bte.cpp.s

.PHONY : bte.s

# target to generate assembly for a file
bte.cpp.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/bte.dir/build.make CMakeFiles/bte.dir/bte.cpp.s
.PHONY : bte.cpp.s

configure.o: configure.cpp.o

.PHONY : configure.o

# target to build an object file
configure.cpp.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/bte.dir/build.make CMakeFiles/bte.dir/configure.cpp.o
.PHONY : configure.cpp.o

configure.i: configure.cpp.i

.PHONY : configure.i

# target to preprocess a source file
configure.cpp.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/bte.dir/build.make CMakeFiles/bte.dir/configure.cpp.i
.PHONY : configure.cpp.i

configure.s: configure.cpp.s

.PHONY : configure.s

# target to generate assembly for a file
configure.cpp.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/bte.dir/build.make CMakeFiles/bte.dir/configure.cpp.s
.PHONY : configure.cpp.s

distribution.o: distribution.cpp.o

.PHONY : distribution.o

# target to build an object file
distribution.cpp.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/bte.dir/build.make CMakeFiles/bte.dir/distribution.cpp.o
.PHONY : distribution.cpp.o

distribution.i: distribution.cpp.i

.PHONY : distribution.i

# target to preprocess a source file
distribution.cpp.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/bte.dir/build.make CMakeFiles/bte.dir/distribution.cpp.i
.PHONY : distribution.cpp.i

distribution.s: distribution.cpp.s

.PHONY : distribution.s

# target to generate assembly for a file
distribution.cpp.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/bte.dir/build.make CMakeFiles/bte.dir/distribution.cpp.s
.PHONY : distribution.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... edit_cache"
	@echo "... rebuild_cache"
	@echo "... bte"
	@echo "... bte.o"
	@echo "... bte.i"
	@echo "... bte.s"
	@echo "... configure.o"
	@echo "... configure.i"
	@echo "... configure.s"
	@echo "... distribution.o"
	@echo "... distribution.i"
	@echo "... distribution.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

