src = $(wildcard src/*.cc)
obj = $(src:.cc=.o) lodepng/lodepng.o
dep = $(obj:.o=.d)
bin = shadertoy

CXXFLAGS = -pedantic -Wall -g -I../libimago/src
LDFLAGS = $(libgl) ../libimago/libimago.a -ldl -lpng -ljpeg

ifeq ($(shell uname -s), Darwin)
	libgl = -framework OpenGL -framework GLUT -lGLEW
else
	libgl = -lGL -lGLU -lglut -lGLEW
endif

$(bin): $(obj)
	$(CXX) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

.cc.d:
	@$(CPP) $(CXXFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
