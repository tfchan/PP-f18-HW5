TARGET = histogram
CXX = g++
CXXFLAGS = -Wall

.PHONY = all clean

all:${TARGET}
${TARGET}:%:%.cpp
	${CXX} ${CXXFLAGS} $? -o $@ -lOpenCL
clean:
	${RM} ${TARGET}
