#
# 'make'        build executable file 'main'
# 'make clean'  removes all .o and executable files
#

# define the Cpp compiler to use
CXX = /Applications/Xcode.app/Contents/Developer/usr/bin/g++
#CXX = /usr/bin/g++

# define any compile-time flags
CXXFLAGS	:= -std=c++17 -Wall -Wextra -g -O0

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
LFLAGS = -ljsoncpp_static

# define output directory
OUTPUT	:= output

# define source directory
SRC		:= src

# define include directory
INCLUDE	:= include

# define lib directory
LIB		:= lib

PCH 	:= $(INCLUDE)/StdInc.hpp
PCH_OUT	:= $(PCH).gch

ifeq ($(OS),Windows_NT)
MAIN	:= is_bjmm_server_busy.exe
SOURCEDIRS	:= $(SRC)
INCLUDEDIRS	:= $(INCLUDE)
LIBDIRS		:= $(LIB)
FIXPATH = $(subst /,\,$1)
RM			:= del /q /f
MD	:= mkdir
else
MAIN	:= is_bjmm_server_busy
SOURCEDIRS	:= $(shell find $(SRC) -type d)
INCLUDEDIRS	:= $(shell find $(INCLUDE) -type d) /opt/local/include
LIBDIRS		:= $(shell find $(LIB) -type d) /opt/local/lib
FIXPATH = $1
RM = rm -f
MD	:= mkdir -p
endif

# define any directories containing header files other than /usr/include
INCLUDES	:= $(patsubst %,-I%, $(INCLUDEDIRS:%/=%))

# define the C libs
LIBS		:= $(patsubst %,-L%, $(LIBDIRS:%/=%))

# define the C source files
SOURCES		:= $(wildcard $(patsubst %,%/*.cpp, $(SOURCEDIRS)))

# define the C object files 
OBJECTS		:= $(SOURCES:.cpp=.o)

#We don't need to clean up when we're making these targets
NODEPS:=clean tags svn
DEPDIR		:= .deps
DEPFLAGS	= -MT $@ -MMD -MP -MF $(DEPDIR)/$(*F).d
#These are the dependency files, which make will clean up after it creates them
#DEPFILES	:= $(SOURCES:%.cpp=$(DEPDIR)/%.d)
DEPFILES	:=	$(patsubst %.cpp,$(DEPDIR)/%.d,$(notdir $(SOURCES)))

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

OUTPUTMAIN	:= $(call FIXPATH,$(OUTPUT)/$(MAIN))

all: $(PCH_OUT) $(OUTPUT) $(DEPDIR) $(MAIN)
	@echo Executing 'all' complete!

$(OUTPUT):
	$(MD) $(OUTPUT)

$(DEPDIR):
	$(MD) $(DEPDIR)

.PHONY: do_clear

$(MAIN): do_clear $(PCH_OUT) $(DEPFILES) $(OBJECTS) 
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(OUTPUTMAIN) $(OBJECTS) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
$(OBJECTS): %.o: %.cpp $(PCH)
	$(CXX) $(DEPFLAGS) $(CXXFLAGS) -include $(PCH) $(INCLUDES) -c $<  -o $@

$(PCH_OUT): $(PCH) Makefile
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

#Don't create dependencies when we're cleaning, for instance
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))

$(DEPFILES):
	#Chances are, these files don't exist.  GMake will create them and
	#clean up automatically afterwards
	-include $(DEPFILES)

endif

do_clear: FORCE
	@clear
FORCE:

.PHONY: clean
clean:
	$(RM) $(OUTPUTMAIN)
	$(RM) $(call FIXPATH,$(OBJECTS))
	$(RM) src/*.d
	@echo Cleanup complete!

run: all
	./$(OUTPUTMAIN)
	@echo Executing 'run: all' complete!

show:
	@echo SOURCES = $(SOURCES)
	@echo OBJECTS = $(OBJECTS)
	@echo DEPFLAGS = $(DEPFLAGS)
	@echo CXXFLAGS = $(CXXFLAGS)
	@echo INCLUDES = $(INCLUDES)
	@echo DEPFILES = $(DEPFILES)
	@echo PCH_OUT = $(PCH_OUT)


