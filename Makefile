########################################
## SETUP MAKEFILE
##      Set the appropriate TARGET (our
## executable name) and any OBJECT files
## we need to compile for this program.
## We would have one object file for every
## cpp file that needs to be compiled and
## linked together.
##
## Next set the path to our local
## include/ and lib/ folders.
## (If you we are compiling in the lab,
## then you can ignore these values.
## They are only for if you are
## compiling on your personal machine.)
##
## Set if we are compiling in the lab
## environment or not.  Set to:
##    1 - if compiling in the Lab
##    0 - if compiling at home
##
########################################

TARGET = A2
OBJECTS = main.o

LOCAL_INC_PATH = /home/zsmeton/Dropbox/Coding/CS444/include
LOCAL_LIB_PATH = /home/zsmeton/Dropbox/Coding/CS444/lib

BUILDING_IN_LAB = 0


#############################
## COMPILING INFO
#############################

CXX    = g++
CFLAGS = -Wall -g -std=c++11

LAB_INC_PATH = Z:/CSCI441/lib
LAB_LIB_PATH = Z:/CSCI441/lib

# if we are not building in the Lab
ifeq ($(BUILDING_IN_LAB), 0)
    # then set our lab paths to our local paths
    # so the Makefile will still work seamlessly
    LAB_INC_PATH = $(LOCAL_INC_PATH)
    LAB_LIB_PATH = $(LOCAL_LIB_PATH)
    CXX = g++
else
	CXX = C:/mingw-w64/mingw64/bin/g++.exe
endif

INCPATH += -I$(LAB_INC_PATH)
LIBPATH += -L$(LAB_LIB_PATH)

#############################
## SETUP SOIL
#############################

LIBS += -lSOIL3

#############################
## SETUP GLEW
#############################

# Windows builds
ifeq ($(OS), Windows_NT)
	LIBS += -lglew32.dll

# Mac builds
else
	ifeq ($(shell uname), Darwin)
		LIBS += -lglew
	# Linux and all other builds
	else
		LIBS += -lOpenGL -lGLEW
	endif
endif

#############################
## SETUP OpenGL & GLFW
#############################

# Windows builds
ifeq ($(OS), Windows_NT)
	LIBS += -lopengl32 -lglfw3 -lgdi32
	TARGET := $(TARGET).exe

# Mac builds
else
	ifeq ($(shell uname), Darwin)
		LIBS += -framework OpenGL -lglfw3 -framework Cocoa -framework IOKit -framework CoreVideo

	# Linux and all other builds
	else
		LIBS += -lGL -lglfw3 -lX11 -lXxf86vm -lXrandr -lpthread -lXi -ldl -lfreetype
	endif
endif



#############################
## COMPILATION INSTRUCTIONS
#############################

all: $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

build: $(OBJECTS)

new: clean $(TARGET)

run: $(TARGET)
	./$(TARGET)

depend:
	rm -f Makefile.bak
	mv Makefile Makefile.bak
	sed '/^# DEPENDENCIES/,$$d' Makefile.bak > Makefile
	echo '# DEPENDENCIES' >> Makefile
	$(CXX) $(INCPATH) -MM *.cpp >> Makefile

.c.o:
	$(CXX) $(CFLAGS) $(INCPATH) -c -o $@ $<

.cc.o:
	$(CXX) $(CFLAGS) $(INCPATH) -c -o $@ $<

.cpp.o:
	$(CXX) $(CFLAGS) $(INCPATH) -c -o $@ $<

$(TARGET): $(OBJECTS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBPATH) $(LIBS)

# DEPENDENCIES
main.o: main.cpp
