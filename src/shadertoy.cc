/*
A native shadertoy-compatible GLSL fragment shader previewer.
Copyright (C) 2013  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <GL/glew.h>
#include <iostream>

#include "../lodepng/lodepng.h"

#ifndef __APPLE__
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif

#include <imago2.h>

struct Texture {
	unsigned int id;
	unsigned int targ;
	const char *stype;
};

void disp();
void idle();
void idle();
void reshape(int x, int y);
void keyb(unsigned char key, int x, int y);
void mouse(int bn, int state, int x, int y);
void motion(int x, int y);
unsigned int load_shader(const char *fname);
bool load_shader_metadata(const char *sdrname);
Texture *load_texture(const char *fname);
Texture *load_cubemap(const char *fname_fmt);
bool parse_args(int argc, char **argv);
void create_texture();
void draw_to_texture();

unsigned int sdr;
static struct {
	int resolution;
	int globaltime;
	int channeltime[4];
	int mouse;
	int sampler[4];
	int date;
} uloc;
std::vector<Texture*> textures;
Texture notex = { 0, GL_TEXTURE_2D, "2D" };
Texture *activetex[4] = {&notex, &notex, &notex, &notex};
int win_width, win_height;
int mouse_x, mouse_y, click_x, click_y;

std::vector<int> tex_arg;
const char *sdrfname_arg;

#define LOADTEX(fname)	\
	do { \
		Texture *tex = load_texture(fname); \
		if(!tex) { \
			return 1; \
		} \
		textures.push_back(tex); \
		printf("  Texture2D %2d: %s\n", dataidx++, fname); \
	} while(0)

#define LOADCUBE(fname) \
	do { \
		Texture *tex = load_cubemap(fname); \
		if(!tex) { \
			return 1; \
		} \
		textures.push_back(tex); \
		printf("TextureCube %2d: %s\n", dataidx++, fname); \
	} while(0)

int main(int argc, char **argv)
{
	glutInitWindowSize(1280, 720);
	glutInit(&argc, argv);

	if(!parse_args(argc, argv)) {
		return 1;
	}

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutCreateWindow("ShaderToy viewer");

	glutDisplayFunc(disp);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyb);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	glewInit();

	int dataidx = 0;

	LOADTEX("data/tex00.jpg");
	LOADTEX("data/tex01.jpg");
	LOADTEX("data/tex02.jpg");
	LOADTEX("data/tex03.jpg");
	LOADTEX("data/tex04.jpg");
	LOADTEX("data/tex05.jpg");
	LOADTEX("data/tex06.jpg");
	LOADTEX("data/tex07.jpg");
	LOADTEX("data/tex08.jpg");
	LOADTEX("data/tex09.jpg");
	LOADTEX("data/tex10.png");
	LOADTEX("data/tex11.png");
	LOADTEX("data/tex12.png");
	LOADTEX("data/tex14.png");
	LOADTEX("data/tex15.png");
	LOADTEX("data/tex16.png");
	LOADTEX("data/tex17.jpg");
	LOADTEX("data/tex18.jpg");
	LOADTEX("data/tex19.png");
	LOADTEX("data/tex20.jpg");
	assert(glGetError() == GL_NO_ERROR);
#if 0
	LOADCUBE("data/cube00_%d.jpg");
	LOADCUBE("data/cube01_%d.png");
	LOADCUBE("data/cube02_%d.jpg");
	LOADCUBE("data/cube03_%d.png");
	LOADCUBE("data/cube04_%d.png");
	LOADCUBE("data/cube05_%d.png");
#endif
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	create_texture();

	load_shader_metadata(sdrfname_arg);

	// override with -t arguments
	for(size_t i=0; i<tex_arg.size(); i++) {
		activetex[i] = textures[tex_arg[i]];
	}

	if(!(sdr = load_shader(sdrfname_arg))) {
		return 1;
	}

	assert(glGetError() == GL_NO_ERROR);
	glutMainLoop();
	return 0;
}

double elapsed_sec = 0.0;
bool paused = false;

void disp()
{
	time_t tmsec = time(0);
	struct tm *tm = localtime(&tmsec);
	struct timespec clk;
	clock_gettime(CLOCK_REALTIME, &clk);
	static struct timespec last_clk = clk;
	// elapsed_sec += 0.015;
	if (!paused) {
		elapsed_sec +=
			(clk.tv_nsec - last_clk.tv_nsec) / 1000.0 / 1000.0 / 1000.0 +
			clk.tv_sec - last_clk.tv_sec;
	}
	last_clk = clk;
	assert(glGetError() == GL_NO_ERROR);

	glClear(GL_COLOR_BUFFER_BIT);

	assert(glGetError() == GL_NO_ERROR);
	glUseProgram(sdr);
	assert(glGetError() == GL_NO_ERROR);

	// set uniforms
	glUniform2f(uloc.resolution, win_width, win_height);
	glUniform1f(uloc.globaltime, elapsed_sec);
	glUniform4f(uloc.mouse, mouse_x, mouse_y, click_x, click_y);
	glUniform4f(uloc.date, tm->tm_year, tm->tm_mon, tm->tm_mday,
			tm->tm_sec + tm->tm_min * 60 + tm->tm_hour * 3600);
	assert(glGetError() == GL_NO_ERROR);

	int tunit = 0;
	for(int i=0; i<4; i++) {
		if(activetex[i]->id) {
			glActiveTexture(GL_TEXTURE0 + tunit);
			glBindTexture(activetex[i]->targ, activetex[i]->id);
			glUniform1i(uloc.sampler[i], tunit);
			tunit++;
		}
	}
	assert(glGetError() == GL_NO_ERROR);

	glBegin(GL_QUADS);
	glVertex2f(-1, -1);
	glVertex2f(1, -1);
	glVertex2f(1, 1);
	glVertex2f(-1, 1);
	glEnd();

	glutSwapBuffers();
	assert(glGetError() == GL_NO_ERROR);
}

void idle()
{
	glutPostRedisplay();
}

void reshape(int x, int y)
{
	glViewport(0, 0, x, y);
	win_width = x;
	win_height = y;
}

void keyb(unsigned char key, int x, int y)
{
	switch(key) {
	case 27:
		exit(0);

	case '{':
			elapsed_sec -= 1.0;
			break;
	case '}':
			elapsed_sec += 1.0;
			break;
	case '[':
			elapsed_sec -= 0.10;
			break;
	case ']':
			elapsed_sec += 0.10;
			break;
	case '<':
			elapsed_sec -= 0.01;
			break;
	case '>':
			elapsed_sec += 0.01;
			break;
	case 'p':
			paused = !paused;
			break;

	case 's':
			draw_to_texture();
			break;

	case 'f':
	case 'F':
		{
			static int orig_width = -1, orig_height;

			if(orig_width != -1) {
				glutReshapeWindow(orig_width, orig_height);
				orig_width = -1;
			} else {
				orig_width = win_width;
				orig_height = win_height;
				glutFullScreen();
			}
		}
		break;
	}
}

void mouse(int bn, int state, int x, int y)
{
	click_x = x;
	click_y = y;
}

void motion(int x, int y)
{
	mouse_x = x;
	mouse_y = y;
}

static const char *header =
	"uniform vec2 iResolution;\n"
	"uniform float iGlobalTime;\n"
	"uniform float iChannelTime[4];\n"
	"uniform vec4 iMouse;\n"
	"uniform sampler%s iChannel0;\n"
	"uniform sampler%s iChannel1;\n"
	"uniform sampler%s iChannel2;\n"
	"uniform sampler%s iChannel3;\n"
	"uniform vec4 iDate;\n";

unsigned int load_shader(const char *fname)
{
	FILE *fp = fopen(fname, "rb");
	if(!fp) {
		fprintf(stderr, "failed to open shader: %s: %s\n", fname, strerror(errno));
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	long filesz = ftell(fp);
	rewind(fp);

	int sz = filesz + strlen(header) + 32;

	const char extra[] = "void main(){mainImage(gl_FragColor, gl_FragCoord.xy);}";
	char *src = new char[sz + sizeof(extra) + 1];
	memset(src, ' ', sz);
	sprintf(src, header, activetex[0]->stype, activetex[1]->stype, activetex[2]->stype, activetex[3]->stype);

	if(fread(src + strlen(src), 1, filesz, fp) < (size_t)filesz) {
		fprintf(stderr, "failed to read shader: %s: %s\n", fname, strerror(errno));
		fclose(fp);
		return 0;
	}
	fclose(fp);

	strcpy(&src[sz], extra);

	printf("compiling shader: %s\n", fname);
	//printf("SOURCE: %s\n", src);
	unsigned int sdr = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(sdr, 1, (const char**)&src, 0);
	glCompileShader(sdr);
	delete [] src;

	int status, loglength;
	glGetShaderiv(sdr, GL_COMPILE_STATUS, &status);
	glGetShaderiv(sdr, GL_INFO_LOG_LENGTH, &loglength);

	if(loglength) {
		char *buf = new char[loglength + 1];
		glGetShaderInfoLog(sdr, loglength, 0, buf);
		buf[loglength] = 0;

		fprintf(stderr, "%s\n", buf);
		delete [] buf;
	}
	if(!status) {
		fprintf(stderr, "failed\n");
		glDeleteShader(sdr);
		return 0;
	}

	unsigned int prog = glCreateProgram();
	glAttachShader(prog, sdr);
	glLinkProgram(prog);

	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &loglength);

	if(loglength) {
		char *buf = new char[loglength + 1];
		glGetProgramInfoLog(prog, loglength, 0, buf);
		buf[loglength] = 0;

		fprintf(stderr, "linking: %s\n", buf);
		delete [] buf;
	}
	if(!status) {
		fprintf(stderr, "status failed\n");
		glDeleteShader(sdr);
		glDeleteProgram(prog);
		return 0;
	}

	glUseProgram(prog);
	assert(glGetError() == GL_NO_ERROR);

	uloc.resolution = glGetUniformLocation(prog, "iResolution");
	uloc.globaltime = glGetUniformLocation(prog, "iGlobalTime");
	uloc.mouse = glGetUniformLocation(prog, "iMouse");
	uloc.date = glGetUniformLocation(prog, "iDate");
	for(int i=0; i<4; i++) {
		char buf[64];
		sprintf(buf, "iChannelTime[%d]", i);
		uloc.channeltime[i] = glGetUniformLocation(prog, buf);
		sprintf(buf, "iChannel%d", i);
		uloc.sampler[i] = glGetUniformLocation(prog, buf);
	}
	return prog;
}

bool load_shader_metadata(const char *sdrname)
{
	// load shader metadata
	int idx = 0;
	char *nbuf = new char[strlen(sdrname) + 6];
	sprintf(nbuf, "%s.meta", sdrname);
	printf("looking for metadata file: %s ...", nbuf);

	FILE *fp = fopen(nbuf, "rb");
	if(!fp) {
		printf("not found\n");
		delete [] nbuf;
		return false;
	}

	printf("found\n");
	char buf[512];
	while(fgets(buf, sizeof buf, fp)) {
		int id;
		if(sscanf(buf, "texture %d", &id) == 1) {
			if(id < 0) {
				activetex[idx++] = &notex;
			} else {
				if(idx < 4) {
					printf("using texture %d in slot %d\n", id, idx);
					activetex[idx++] = textures[id];
				}
			}
		}
	}
	fclose(fp);
	delete [] nbuf;
	return true;
}

Texture *load_texture(const char *fname)
{
	Texture *tex = new Texture;
	tex->id = img_gltexture_load(fname);
	if(!tex->id) {
		fprintf(stderr, "failed to load texture: %s\n", fname);
		return 0;
	}
	tex->targ = GL_TEXTURE_2D;
	tex->stype = "2D";
	return tex;
}

Texture *load_cubemap(const char *fname_fmt)
{
	char *fname = new char[strlen(fname_fmt) + 1];

	Texture *tex = new Texture;
	tex->targ = GL_TEXTURE_CUBE_MAP;
	tex->stype = "Cube";

	glGenTextures(1, &tex->id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex->id);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	assert(glGetError() == GL_NO_ERROR);

	int first_xsz = 0, first_ysz = 0;

	for(int i=0; i<6; i++) {
		int xsz, ysz;
		sprintf(fname, fname_fmt, i);
		void *pixels = img_load_pixels(fname, &xsz, &ysz, IMG_FMT_RGBAF);
		if(!pixels) {
			fprintf(stderr, "failed to load image: %s\n", fname);
			glDeleteTextures(1, &tex->id);
			delete tex;
			return 0;
		}

		if(i == 0) {
			first_xsz = xsz;
			first_ysz = ysz;
		} else {
			if(first_xsz != xsz || first_ysz != ysz) {
				fprintf(stderr, "cubemap face %s isn't %dx%d like the rest\n", fname, first_xsz, first_ysz);
				img_free_pixels(pixels);
				glDeleteTextures(1, &tex->id);
				delete tex;
				return 0;
			}
		}

		assert(glGetError() == GL_NO_ERROR);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, xsz, ysz, 0, GL_RGBA, GL_FLOAT, pixels);
		assert(glGetError() == GL_NO_ERROR);
		img_free_pixels(pixels);
	}

	return tex;
}

bool parse_args(int argc, char **argv)
{
	for(int i=0; i<argc; i++) {
		if(argv[i][0] == '-') {
			switch(argv[i][1]) {
			case 't':
				if(tex_arg.size() >= 4) {
					fprintf(stderr, "too many textures specified\n");
					return false;
				} else {
					char *endp;
					int idx = strtol(argv[++i], &endp, 10);
					if(endp == argv[i]) {
						fprintf(stderr, "-t must be followed by a number\n");
						return false;
					}
					tex_arg.push_back(idx);
				}
				break;

			default:
				fprintf(stderr, "unrecognized option: %s\n", argv[i]);
				return false;
			}
		} else {
			if(sdrfname_arg) {
				fprintf(stderr, "too many shaders specified\n");
				return false;
			}
			sdrfname_arg = argv[++i];
		}
	}
	return true;
}

static GLubyte *capture_pixels = NULL;
static GLuint framebuffer = 0;
static const GLuint DPI = 300;
static const GLuint CAPTURE_WIDTH = DPI*36;
static const GLuint CAPTURE_HEIGHT = DPI*24;
static const GLenum CAPTURE_FORMAT = GL_RGBA;
static const GLuint CAPTURE_FORMAT_NBYTES = 4;

static void save_image() {
  static unsigned int nscreenshots = 0;
  char filename[256];
  snprintf(filename, sizeof(filename), "screenshot%d.png", nscreenshots++);

  std::cout << "Capturing " << filename << "..." << std::flush;
  std::vector<unsigned char> image_buf;
  lodepng::encode(image_buf, capture_pixels, CAPTURE_WIDTH, CAPTURE_HEIGHT);
  lodepng::save_file(image_buf, filename);
  std::cout << "done" << std::endl;
}

void create_texture()
{
  // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  // The texture we're going to render to
  GLuint renderedTexture;
  glGenTextures(1, &renderedTexture);

  // "Bind" the newly created texture : all future texture functions will modify this texture
  glBindTexture(GL_TEXTURE_2D, renderedTexture);

  // Give an empty image to OpenGL ( the last "0" )
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CAPTURE_WIDTH, CAPTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  assert(glGetError() == GL_NO_ERROR);

  // Poor filtering. Needed !
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  // The depth buffer
  GLuint depthrenderbuffer;
  glGenRenderbuffers(1, &depthrenderbuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, CAPTURE_WIDTH, CAPTURE_HEIGHT);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

  // Set "renderedTexture" as our colour attachement #0
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0);

  // Set the list of draw buffers.
  GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

  // Always check that our framebuffer is ok
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    return;

  capture_pixels = (GLubyte*)malloc(CAPTURE_FORMAT_NBYTES * CAPTURE_WIDTH * CAPTURE_HEIGHT);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void draw_to_texture()
{
	int orig_width = win_width, orig_height = win_height;
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	reshape(CAPTURE_WIDTH, CAPTURE_HEIGHT);
	assert(glGetError() == GL_NO_ERROR);

	disp();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glReadPixels(0, 0, CAPTURE_WIDTH, CAPTURE_HEIGHT, CAPTURE_FORMAT, GL_UNSIGNED_BYTE, capture_pixels);
	assert(glGetError() == GL_NO_ERROR);

	save_image();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	reshape(orig_width, orig_height);
}
