#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#include "glew.h"
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include "glut.h"
//#include "glui.h"
#include "glslprogram.h"
#include "glui.h"

#define MS_PER_CYCLE	100
#define S_PER_CYCLE		1000
float	radius = 2.0;
int		stacks = 40;
int		slices = 340;

// title of these windows:

const char* WINDOWTITLE = { "OpenGL / GLUT Sample -- Joe Graphics" };
const char* GLUITITLE = { "User Interface Window" };

const float NUMFIRES = { 100000 };
const float DT = { 0.030f };
const float GRAVITY = { -10. };

const float XL = -30.f;
const float XR = 30.f;
const float YB = -55.f;
const float YT = 5.f;
// what the glui package defines as true and false:

const int GLUITRUE = { true };
const int GLUIFALSE = { false };
int	DoFan;
int	DoTraces;
GLSLProgram* Pattern;
//GLSLProgram* Pattern2;
float		 Time;
float	TransXYZ[3];		// set by glui translation widgets
// the escape key:

#define ESCAPE		0x1b
#define NVIDIA_SHADER_BINARY	0x00008e21		// nvidia binary enum

float	Distort = 0.0;		// global -- true means to distort the texture

int	ReadInt(FILE*);
short ReadShort(FILE*);


struct bmfh
{
	short bfType;
	int bfSize;
	short bfReserved1;
	short bfReserved2;
	int bfOffBits;
} FileHeader;

struct bmih
{
	int biSize;
	int biWidth;
	int biHeight;
	short biPlanes;
	short biBitCount;
	int biCompression;
	int biSizeImage;
	int biXPelsPerMeter;
	int biYPelsPerMeter;
	int biClrUsed;
	int biClrImportant;
} InfoHeader;


struct GLshadertype
{
	char* extension;
	GLenum name;
}
ShaderTypes[] =
{
	{ ".cs",   GL_COMPUTE_SHADER },
	{ ".vert", GL_VERTEX_SHADER },
	{ ".vs",   GL_VERTEX_SHADER },
	{ ".frag", GL_FRAGMENT_SHADER },
	{ ".fs",   GL_FRAGMENT_SHADER },
	{ ".geom", GL_GEOMETRY_SHADER },
	{ ".gs",   GL_GEOMETRY_SHADER },
	{ ".tcs",  GL_TESS_CONTROL_SHADER },
	{ ".tes",  GL_TESS_EVALUATION_SHADER },
};

struct GLbinarytype
{
	char* extension;
	GLenum format;
}
BinaryTypes[] =
{
	{ ".nvb",    NVIDIA_SHADER_BINARY },
};

extern char* Gstap;		// set later

static
char*
GetExtension(char* file)
{
	int n = (int)strlen(file) - 1;	// index of last non-null character

	// look for a '.':

	do
	{
		if (file[n] == '.')
			return &file[n];	// the extension includes the '.'
		n--;
	} while (n >= 0);

	// never found a '.':

	return NULL;
}


// initial window size:

const int INIT_WINDOW_SIZE = { 600 };


// size of the box:

const float BOXSIZE = { 2.f };

const float XMIN = { -100.0 };
const float XMAX = { 100.0 };
const float YMIN = { -100.0 };
const float YMAX = { 100.0 };
const float ZMIN = { -100.0 };
const float ZMAX = { 100.0 };
const float RMIN = { 2.f };
const float RMAX = { 6.f };
const float RADIUS = { 3.f };
const float TMIN = { 0.0 };
const float TMAX = { 50.0 };
const float THETAMIN = { (float)(M_PI) / 4.f };
const float THETAMAX = { 3.f * (float)(M_PI) / 4.f };
const float VELMIN = { 10.f };
const float VELMAX = { 40.f };


// multiplication factors for input interaction:
//  (these are known from previous experience)

const float ANGFACT = { 1. };
const float SCLFACT = { 0.005f };


// minimum allowable scale factor:

const float MINSCALE = { 0.05f };


// active mouse buttons (or them together):

const int LEFT = { 4 };
const int MIDDLE = { 2 };
const int RIGHT = { 1 };


// which projection:

enum Projections
{
	ORTHO,
	PERSP
};

// which button:
enum ButtonVals
{
	RESET,
	QUIT
};


// window background color (rgba):

const GLfloat BACKCOLOR[] = { 0., 0., 0., 1. };


// line width for the axes:
const GLfloat AXES_COLOR[] = { 1., .5, 0. };
const GLfloat AXES_WIDTH = { 3. };

// fog parameters:
#define NUMTRACES		30
const GLfloat FOGCOLOR[4] = { .0, .0, .0, 1. };
const GLenum  FOGMODE = { GL_LINEAR };
const GLfloat FOGDENSITY = { 0.30f };
const GLfloat FOGSTART = { 1.5 };
const GLfloat FOGEND = { 4. };
// the color numbers:
// this order must match the radio button order

enum Colors
{
	RED,
	YELLOW,
	GREEN,
	CYAN,
	BLUE,
	MAGENTA,
	WHITE,
	BLACK
};

char* ColorNames[] =
{
	"Red",
	"Yellow",
	"Green",
	"Cyan",
	"Blue",
	"Magenta",
	"White",
	"Black"
};


// the color definitions:
// this order must match the menu order

const GLfloat Colors[][3] =
{
	{ 1., 0., 0. },		// red
	{ 1., 1., 0. },		// yellow
	{ 0., 1., 0. },		// green
	{ 0., 1., 1. },		// cyan
	{ 0., 0., 1. },		// blue
	{ 1., 0., 1. },		// magenta
	{ 1., 1., 1. },		// white
	{ 0., 0., 0. },		// black
};

struct state
{
	float y;
	float vy;
	float x;
	float vx;
};

struct derivatives
{
	float vy;
	float ay;
	float vx;
	float ax;
};

struct fire
{
	float x0, y0, z0;	// starting location	
	float vx0, vy0, vz0;
	float vx, vy, vz;	// velocity		
	float r0, g0, b0;	// starting color
	float t0, t1;		// start, end time	
	float x, y, z;		// current location	
	float rad;		// radius		
	float r, g, b;		// color		
	int ti;			// trace index	
	int numt;		// #trace elements that are valid
	float tx[NUMTRACES], ty[NUMTRACES], tz[NUMTRACES];	// trace coords
};

#define NUMNODES	30

struct state		State[NUMNODES];

// fog parameters:

float  Scale2;		// scaling factors
float AA, BB, CC, DD;

// non-constant global variables:

int		ActiveButton;			// current button that is down
GLuint	AxesList;				// list to hold the axes
int		AxesOn;					// != 0 means to draw the axes
int		DebugOn;				// != 0 means to print debugging info
int		DepthCueOn;				// != 0 means to use intensity depth cueing
int		DepthBufferOn;			// != 0 means to use the z-buffer
int		DepthFightingOn;		// != 0 means to use the z-buffer
float	Dt;
struct fire* Fire;		// array of structures	
GLuint	BoxList;				// object display list
GLuint	SphereList;				// Sphere display list
GLUI* Glui;			// instance of glui window
int	GluiWindow;		// the glut id for the glui window
int		MainWindow;				// window id for main graphics window
float	Scale;					// scaling factor
int		WhichColor;				// index into Colors[ ]
int		WhichProjection;		// ORTHO or PERSP
int		Xmouse, Ymouse;			// mouse values
float	Xrot, Yrot;				// rotation angles in degrees
bool	vertexshader = true;			// turn on/off vertex
bool	fragmentshader = true;			// turn on/off frag
bool	FreezeAnimation;		// free the animation
float	testtime;
bool	BothAnimation;
int	width = 1024;
int height = 512;
float	myTime, myTime1, myTime2, myTime3;
int	Debug;			// ON means print debug info
GLfloat	RotMatrix[4][4];	// set by glui rotation widget
// function prototypes:

void	Animate();
void	Display();
void	DoAxesMenu(int);
void	DoColorMenu(int);
void	DoDepthBufferMenu(int);
void	DoDepthFightingMenu(int);
void	DoDepthMenu(int);
void	DoDebugMenu(int);
void	DoMainMenu(int);
void	DoProjectMenu(int);
void	DoRasterString(float, float, float, char*);
void	DoStrokeString(float, float, float, float, char*);
float	ElapsedSeconds();
void	InitGlui(void);
void	InitGraphics();
//unsigned char* BmpToTexture(char* filename, int* width, int* height);
void	InitLists();
void	InitList();
void	MakeSphere(float, int, int);
void	InitMenus();
void	Keyboard(unsigned char, int, int);
void	MouseButton(int, int, int, int);
void	MouseMotion(int, int);
void	Reset();
void	Resize(int, int);
void	Visibility(int);
int		ReadInt(FILE* fp);
short	ReadShort(FILE* fp);
void	Axes(float);
void	HsvRgb(float[3], float[3]);
float	LavaTime;
unsigned char* Texture1;
const int birgb = { 0 };

float uAA, uPP, uTol, uNoiseAmp, uNoiseFreq, alpha, ublue, ured;
bool uUseChromadepth = false;
bool uAn, uBn, uNoisen, uNoisefn = false;
float changemore = 0.0f;
int	NumFires;		// # fires		
int	ParticleList;
int	LeftButton;	

struct point {
	float x, y, z;		// coordinates
	float nx, ny, nz;	// surface normal
	float s, t;		// texture coords
};

static int		NumLngs, NumLats;
static struct point* Pts;

inline struct point*
PtsPointer(int lat, int lng)
{
	if (lat < 0)	lat += (NumLats - 1);
	if (lng < 0)	lng += (NumLngs - 1);
	if (lat > NumLats - 1)	lat -= (NumLats - 1);
	if (lng > NumLngs - 1)	lng -= (NumLngs - 1);
	return &Pts[NumLngs * lat + lng];
}

inline void
DrawPoint(struct point* p)
{
	glNormal3f(p->nx, p->ny, p->nz);
	if (p->nx < 1.0 && p->nx >0) {
		p->s = 0.5;
		p->t = 0.5;
	}
	glTexCoord2f(p->s, p->t);
	glVertex3f(p->x, p->y, p->z);
}


GLSLProgram::GLSLProgram()
{
	Verbose = false;
	InputTopology = GL_TRIANGLES;
	OutputTopology = GL_TRIANGLE_STRIP;

	CanDoComputeShaders = IsExtensionSupported("GL_ARB_compute_shader");
	CanDoVertexShaders = IsExtensionSupported("GL_ARB_vertex_shader");
	CanDoTessControlShaders = IsExtensionSupported("GL_ARB_tessellation_shader");
	CanDoTessEvaluationShaders = CanDoTessControlShaders;
	CanDoGeometryShaders = IsExtensionSupported("GL_EXT_geometry_shader4");
	CanDoFragmentShaders = IsExtensionSupported("GL_ARB_fragment_shader");
	CanDoBinaryFiles = IsExtensionSupported("GL_ARB_get_program_binary");

	fprintf(stderr, "Can do: ");
	if (CanDoComputeShaders)		fprintf(stderr, "compute shaders, ");
	if (CanDoVertexShaders)		fprintf(stderr, "vertex shaders, ");
	if (CanDoTessControlShaders)		fprintf(stderr, "tess control shaders, ");
	if (CanDoTessEvaluationShaders)	fprintf(stderr, "tess evaluation shaders, ");
	if (CanDoGeometryShaders)		fprintf(stderr, "geometry shaders, ");
	if (CanDoFragmentShaders)		fprintf(stderr, "fragment shaders, ");
	if (CanDoBinaryFiles)			fprintf(stderr, "binary shader files ");
	fprintf(stderr, "\n");
}

enum LeftButton
{
	ROTATE,
	SCALE
};
// this is what is exposed to the user
// file1 - file5 are defaulted as NULL if not given
// CreateHelper is a varargs procedure, so must end in a NULL argument,
//	which I know to supply but I'm worried users won't

bool
GLSLProgram::Create(char* file0, char* file1, char* file2, char* file3, char* file4, char* file5)
{
	return CreateHelper(file0, file1, file2, file3, file4, file5, NULL);
}

//GLuint	BoxList;				// object display list
GLuint	tex1;

// this is the varargs version of the Create method

bool
GLSLProgram::CreateHelper(char* file0, ...)
{
	GLsizei n = 0;
	GLchar* buf;
	Valid = true;

	IncludeGstap = false;
	Cshader = Vshader = TCshader = TEshader = Gshader = Fshader = 0;
	Program = 0;
	AttributeLocs.clear();
	UniformLocs.clear();

	if (Program == 0)
	{
		Program = glCreateProgram();
		CheckGlErrors("glCreateProgram");
	}

	va_list args;
	va_start(args, file0);

	// This is a little dicey
	// There is no way, using var args, to know how many arguments were passed
	// I am depending on the caller passing in a NULL as the final argument.
	// If they don't, bad things will happen.

	char* file = file0;
	int type;
	while (file != NULL)
	{
		int maxBinaryTypes = sizeof(BinaryTypes) / sizeof(struct GLbinarytype);
		type = -1;
		char* extension = GetExtension(file);
		// fprintf( stderr, "File = '%s', extension = '%s'\n", file, extension );

		for (int i = 0; i < maxBinaryTypes; i++)
		{
			if (strcmp(extension, BinaryTypes[i].extension) == 0)
			{
				// fprintf( stderr, "Legal extension = '%s'\n", extension );
				LoadProgramBinary(file, BinaryTypes[i].format);
				break;
			}
		}

		int maxShaderTypes = sizeof(ShaderTypes) / sizeof(struct GLshadertype);
		for (int i = 0; i < maxShaderTypes; i++)
		{
			if (strcmp(extension, ShaderTypes[i].extension) == 0)
			{
				// fprintf( stderr, "Legal extension = '%s'\n", extension );
				type = i;
				break;
			}
		}

		GLuint shader;
		bool SkipToNextVararg = false;
		if (type < 0)
		{
			fprintf(stderr, "Unknown filename extension: '%s'\n", extension);
			fprintf(stderr, "Legal Extensions are: ");
			for (int i = 0; i < maxBinaryTypes; i++)
			{
				if (i != 0)	fprintf(stderr, " , ");
				fprintf(stderr, "%s", BinaryTypes[i].extension);
			}
			fprintf(stderr, "\n");
			for (int i = 0; i < maxShaderTypes; i++)
			{
				if (i != 0)	fprintf(stderr, " , ");
				fprintf(stderr, "%s", ShaderTypes[i].extension);
			}
			fprintf(stderr, "\n");
			Valid = false;
			SkipToNextVararg = true;
		}

		if (!SkipToNextVararg)
		{
			switch (ShaderTypes[type].name)
			{
			case GL_COMPUTE_SHADER:
				if (!CanDoComputeShaders)
				{
					fprintf(stderr, "Warning: this system cannot handle compute shaders\n");
					Valid = false;
					SkipToNextVararg = true;
				}
				else
				{
					shader = glCreateShader(GL_COMPUTE_SHADER);
				}
				break;

			case GL_VERTEX_SHADER:
				if (!CanDoVertexShaders)
				{
					fprintf(stderr, "Warning: this system cannot handle vertex shaders\n");
					Valid = false;
					SkipToNextVararg = true;
				}
				else
				{
					shader = glCreateShader(GL_VERTEX_SHADER);
				}
				break;

			case GL_TESS_CONTROL_SHADER:
				if (!CanDoTessControlShaders)
				{
					fprintf(stderr, "Warning: this system cannot handle tessellation control shaders\n");
					Valid = false;
					SkipToNextVararg = true;
				}
				else
				{
					shader = glCreateShader(GL_TESS_CONTROL_SHADER);
				}
				break;

			case GL_TESS_EVALUATION_SHADER:
				if (!CanDoTessEvaluationShaders)
				{
					fprintf(stderr, "Warning: this system cannot handle tessellation evaluation shaders\n");
					Valid = false;
					SkipToNextVararg = true;
				}
				else
				{
					shader = glCreateShader(GL_TESS_EVALUATION_SHADER);
				}
				break;

			case GL_GEOMETRY_SHADER:
				if (!CanDoGeometryShaders)
				{
					fprintf(stderr, "Warning: this system cannot handle geometry shaders\n");
					Valid = false;
					SkipToNextVararg = true;
				}
				else
				{
					//glProgramParameteriEXT( Program, GL_GEOMETRY_INPUT_TYPE_EXT,  InputTopology );
					//glProgramParameteriEXT( Program, GL_GEOMETRY_OUTPUT_TYPE_EXT, OutputTopology );
					//glProgramParameteriEXT( Program, GL_GEOMETRY_VERTICES_OUT_EXT, 1024 );
					shader = glCreateShader(GL_GEOMETRY_SHADER);
				}
				break;

			case GL_FRAGMENT_SHADER:
				if (!CanDoFragmentShaders)
				{
					fprintf(stderr, "Warning: this system cannot handle fragment shaders\n");
					Valid = false;
					SkipToNextVararg = true;
				}
				else
				{
					shader = glCreateShader(GL_FRAGMENT_SHADER);
				}
				break;
			}
		}


		// read the shader source into a buffer:

		if (!SkipToNextVararg)
		{
			FILE* in;
			int length;
			FILE* logfile;

			in = fopen(file, "rb");
			if (in == NULL)
			{
				fprintf(stderr, "Cannot open shader file '%s'\n", file);
				Valid = false;
				SkipToNextVararg = true;
			}

			if (!SkipToNextVararg)
			{
				fseek(in, 0, SEEK_END);
				length = ftell(in);
				fseek(in, 0, SEEK_SET);		// rewind

				buf = new GLchar[length + 1];
				fread(buf, sizeof(GLchar), length, in);
				buf[length] = '\0';
				fclose(in);

				GLchar* strings[2];
				int n = 0;

				if (IncludeGstap)
				{
					strings[n] = Gstap;
					n++;
				}

				strings[n] = buf;
				n++;

				// Tell GL about the source:

				glShaderSource(shader, n, (const GLchar**)strings, NULL);
				delete[] buf;
				CheckGlErrors("Shader Source");

				// compile:

				glCompileShader(shader);
				GLint infoLogLen;
				GLint compileStatus;
				CheckGlErrors("CompileShader:");
				glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

				if (compileStatus == 0)
				{
					fprintf(stderr, "Shader '%s' did not compile.\n", file);
					glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
					if (infoLogLen > 0)
					{
						GLchar* infoLog = new GLchar[infoLogLen + 1];
						glGetShaderInfoLog(shader, infoLogLen, NULL, infoLog);
						infoLog[infoLogLen] = '\0';
						logfile = fopen("glsllog.txt", "w");
						if (logfile != NULL)
						{
							fprintf(logfile, "\n%s\n", infoLog);
							fclose(logfile);
						}
						fprintf(stderr, "\n%s\n", infoLog);
						delete[] infoLog;
					}
					glDeleteShader(shader);
					Valid = false;
				}
				else
				{
					if (Verbose)
						fprintf(stderr, "Shader '%s' compiled.\n", file);

					glAttachShader(this->Program, shader);
				}
			}
		}



		// go to the next vararg file:

		file = va_arg(args, char*);
	}

	va_end(args);

	// link the entire shader program:

	glLinkProgram(Program);
	CheckGlErrors("Link Shader 1");

	GLchar* infoLog;
	GLint infoLogLen;
	GLint linkStatus;
	glGetProgramiv(this->Program, GL_LINK_STATUS, &linkStatus);
	CheckGlErrors("Link Shader 2");

	if (linkStatus == 0)
	{
		glGetProgramiv(this->Program, GL_INFO_LOG_LENGTH, &infoLogLen);
		fprintf(stderr, "Failed to link program -- Info Log Length = %d\n", infoLogLen);
		if (infoLogLen > 0)
		{
			infoLog = new GLchar[infoLogLen + 1];
			glGetProgramInfoLog(this->Program, infoLogLen, NULL, infoLog);
			infoLog[infoLogLen] = '\0';
			fprintf(stderr, "Info Log:\n%s\n", infoLog);
			delete[] infoLog;

		}
		glDeleteProgram(Program);
		Valid = false;
	}
	else
	{
		if (Verbose)
			fprintf(stderr, "Shader Program linked.\n");
		// validate the program:

		GLint status;
		glValidateProgram(Program);
		glGetProgramiv(Program, GL_VALIDATE_STATUS, &status);
		if (status == GL_FALSE)
		{
			fprintf(stderr, "Program is invalid.\n");
			Valid = false;
		}
		else
		{
			if (Verbose)
				fprintf(stderr, "Shader Program validated.\n");
		}
	}

	return Valid;
}


void
GLSLProgram::DispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z)
{
	Use();
	glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
}


bool
GLSLProgram::IsValid()
{
	return Valid;
}


bool
GLSLProgram::IsNotValid()
{
	return !Valid;
}


void
GLSLProgram::SetVerbose(bool v)
{
	Verbose = v;
}


void
GLSLProgram::Use()
{
	Use(this->Program);
};


void
GLSLProgram::Use(GLuint p)
{
	if (p != CurrentProgram)
	{
		glUseProgram(p);
		CurrentProgram = p;
	}
};


void
GLSLProgram::UseFixedFunction()
{
	this->Use(0);
};


int
GLSLProgram::GetAttributeLocation(char* name)
{
	std::map<char*, int>::iterator pos;

	pos = AttributeLocs.find(name);
	if (pos == AttributeLocs.end())
	{
		AttributeLocs[name] = glGetAttribLocation(this->Program, name);
	}

	return AttributeLocs[name];
};


#ifdef NOT_SUPPORTED
void
GLSLProgram::SetAttributeVariable(char* name, int val)
{
	int loc;
	if ((loc = GetAttributeLocation(name)) >= 0)
	{
		this->Use();
		glVertexAttrib1i(loc, val);
	}
};
#endif


void
GLSLProgram::SetAttributeVariable(char* name, float val)
{
	int loc;
	if ((loc = GetAttributeLocation(name)) >= 0)
	{
		this->Use();
		glVertexAttrib1f(loc, val);
	}
};


void
GLSLProgram::SetAttributeVariable(char* name, float val0, float val1, float val2)
{
	int loc;
	if ((loc = GetAttributeLocation(name)) >= 0)
	{
		this->Use();
		glVertexAttrib3f(loc, val0, val1, val2);
	}
};

unsigned char*
BmpToTexture(char* filename, int* width, int* height)
{

	int s, t, e;		// counters
	int numextra;		// # extra bytes each line in the file is padded with
	FILE* fp;
	unsigned char* texture;
	int nums, numt;
	unsigned char* tp;


	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "Cannot open Bmp file '%s'\n", filename);
		return NULL;
	}

	FileHeader.bfType = ReadShort(fp);


	// if bfType is not 0x4d42, the file is not a bmp:

	if (FileHeader.bfType != 0x4d42)
	{
		fprintf(stderr, "Wrong type of file: 0x%0x\n", FileHeader.bfType);
		fclose(fp);
		return NULL;
	}


	FileHeader.bfSize = ReadInt(fp);
	FileHeader.bfReserved1 = ReadShort(fp);
	FileHeader.bfReserved2 = ReadShort(fp);
	FileHeader.bfOffBits = ReadInt(fp);


	InfoHeader.biSize = ReadInt(fp);
	InfoHeader.biWidth = ReadInt(fp);
	InfoHeader.biHeight = ReadInt(fp);

	nums = InfoHeader.biWidth;
	numt = InfoHeader.biHeight;

	InfoHeader.biPlanes = ReadShort(fp);
	InfoHeader.biBitCount = ReadShort(fp);
	InfoHeader.biCompression = ReadInt(fp);
	InfoHeader.biSizeImage = ReadInt(fp);
	InfoHeader.biXPelsPerMeter = ReadInt(fp);
	InfoHeader.biYPelsPerMeter = ReadInt(fp);
	InfoHeader.biClrUsed = ReadInt(fp);
	InfoHeader.biClrImportant = ReadInt(fp);


	// fprintf( stderr, "Image size found: %d x %d\n", ImageWidth, ImageHeight );


	texture = new unsigned char[3 * nums * numt];
	if (texture == NULL)
	{
		fprintf(stderr, "Cannot allocate the texture array!\b");
		return NULL;
	}


	// extra padding bytes:

	numextra = 4 * (((3 * InfoHeader.biWidth) + 3) / 4) - 3 * InfoHeader.biWidth;


	// we do not support compression:

	if (InfoHeader.biCompression != birgb)
	{
		fprintf(stderr, "Wrong type of image compression: %d\n", InfoHeader.biCompression);
		fclose(fp);
		return NULL;
	}



	rewind(fp);
	fseek(fp, 14 + 40, SEEK_SET);

	if (InfoHeader.biBitCount == 24)
	{
		for (t = 0, tp = texture; t < numt; t++)
		{
			for (s = 0; s < nums; s++, tp += 3)
			{
				*(tp + 2) = fgetc(fp);		// b
				*(tp + 1) = fgetc(fp);		// g
				*(tp + 0) = fgetc(fp);		// r
			}

			for (e = 0; e < numextra; e++)
			{
				fgetc(fp);
			}
		}
	}

	fclose(fp);

	*width = nums;
	*height = numt;
	return texture;
}


void
GLSLProgram::SetAttributeVariable(char* name, float vals[3])
{
	int loc;
	if ((loc = GetAttributeLocation(name)) >= 0)
	{
		this->Use();
		glVertexAttrib3fv(loc, vals);
	}
};


#ifdef VEC3_H
void
GLSLProgram::SetAttributeVariable(char* name, Vec3& v);
{
	int loc;
	if ((loc = GetAttributeLocation(name)) >= 0)
	{
		float vec[3];
		v.GetVec3(vec);
		this->Use();
		glVertexAttrib3fv(loc, 3, vec);
	}
};
#endif


#ifdef VERTEX_BUFFER_OBJECT_H
void
GLSLProgram::SetAttributeVariable(char* name, VertexBufferObject& vb, GLenum which)
{
	int loc;
	if ((loc = GetAttributeLocation(name)) >= 0)
	{
		this->Use();
		glEnableVertexAttribArray(loc);
		switch (which)
		{
		case GL_VERTEX:
			glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(? ));
			break;

		case GL_NORMAL:
			glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(? ));
			break;

		case GL_COLOR:
			glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(? ));
			break;
		}
	};
#endif

	int
		GLSLProgram::GetUniformLocation(char* name)
	{
		std::map<char*, int>::iterator pos;

		pos = UniformLocs.find(name);
		//if( Verbose )
			//fprintf( stderr, "Uniform: pos = 0x%016x ; size = %d ; end = 0x%016x\n", pos, UniformLocs.size(), UniformLocs.end() );
		if (pos == UniformLocs.end())
		{
			GLuint loc = glGetUniformLocation(this->Program, name);
			UniformLocs[name] = loc;
			if (Verbose)
				fprintf(stderr, "Location of '%s' in Program %d = %d\n", name, this->Program, loc);
		}
		else
		{
			if (Verbose)
			{
				fprintf(stderr, "Location = %d\n", UniformLocs[name]);
				if (UniformLocs[name] == -1)
					fprintf(stderr, "Location of uniform variable '%s' is -1\n", name);
			}
		}

		return UniformLocs[name];
	};

	void
		GLSLProgram::SetUniformVariable(char* name, int val)
	{
		int loc;
		if ((loc = GetUniformLocation(name)) >= 0)
		{
			this->Use();
			glUniform1i(loc, val);
		}
	};

	void
		GLSLProgram::SetUniformVariable(char* name, float val)
	{
		int loc;
		if ((loc = GetUniformLocation(name)) >= 0)
		{
			this->Use();
			glUniform1f(loc, val);
		}
	};
	void
		GLSLProgram::SetUniformVariable(char* name, float val0, float val1, float val2)
	{
		int loc;
		if ((loc = GetUniformLocation(name)) >= 0)
		{
			this->Use();
			glUniform3f(loc, val0, val1, val2);
		}
	};
	void
		GLSLProgram::SetUniformVariable(char* name, float vals[3])
	{
		int loc;
		fprintf(stderr, "Found a 3-element array\n");

		if ((loc = GetUniformLocation(name)) >= 0)
		{
			this->Use();
			glUniform3fv(loc, 3, vals);
		}
	};

#ifdef VEC3_H
	void
		GLSLProgram::SetUniformVariable(char* name, Vec3 & v);
	{
		int loc;
		if ((loc = GetAttributeLocation(name)) >= 0)
		{
			float vec[3];
			v.GetVec3(vec);
			this->Use();
			glUniform3fv(loc, 3, vec);
		}
	};
#endif


#ifdef MATRIX4_H
	void
		GLSLProgram::SetUniformVariable(char* name, Matrix4 & m)
	{
		int loc;
		if ((loc = GetUniformLocation(name)) >= 0)
		{
			float mat[4][4];
			m.GetMatrix4(mat);
			this->Use();
			glUniformMatrix4fv(loc, 16, true, mat);
		}
	};
#endif

	void
		GLSLProgram::SetInputTopology(GLenum t)
	{
		if (t != GL_POINTS && t != GL_LINES && t != GL_LINES_ADJACENCY_EXT && t != GL_TRIANGLES && t != GL_TRIANGLES_ADJACENCY_EXT)
		{
			fprintf(stderr, "Warning: You have not specified a supported Input Topology\n");
		}
		InputTopology = t;
	}

	void
		GLSLProgram::SetOutputTopology(GLenum t)
	{
		if (t != GL_POINTS && t != GL_LINE_STRIP && t != GL_TRIANGLE_STRIP)
		{
			fprintf(stderr, "Warning: You have not specified a supported Onput Topology\n");
		}
		OutputTopology = t;
	}

	bool
		GLSLProgram::IsExtensionSupported(const char* extension)
	{
		// see if the extension is bogus:

		if (extension == NULL || extension[0] == '\0')
			return false;

		GLubyte* where = (GLubyte*)strchr(extension, ' ');
		if (where != 0)
			return false;

		// get the full list of extensions:

		const GLubyte* extensions = glGetString(GL_EXTENSIONS);

		for (const GLubyte* start = extensions; ; )
		{
			where = (GLubyte*)strstr((const char*)start, extension);
			if (where == 0)
				return false;

			GLubyte* terminator = where + strlen(extension);

			if (where == start || *(where - 1) == '\n' || *(where - 1) == ' ')
				if (*terminator == ' ' || *terminator == '\n' || *terminator == '\0')
					return true;
			start = terminator;
		}
		return false;
	}

	int GLSLProgram::CurrentProgram = 0;

#ifndef CHECK_GL_ERRORS
#define CHECK_GL_ERRORS
	void
		CheckGlErrors(const char* caller)
	{
		unsigned int gle = glGetError();

		if (gle != GL_NO_ERROR)
		{
			fprintf(stderr, "GL Error discovered from caller %s: ", caller);
			switch (gle)
			{
			case GL_INVALID_ENUM:
				fprintf(stderr, "Invalid enum.\n");
				break;
			case GL_INVALID_VALUE:
				fprintf(stderr, "Invalid value.\n");
				break;
			case GL_INVALID_OPERATION:
				fprintf(stderr, "Invalid Operation.\n");
				break;
			case GL_STACK_OVERFLOW:
				fprintf(stderr, "Stack overflow.\n");
				break;
			case GL_STACK_UNDERFLOW:
				fprintf(stderr, "Stack underflow.\n");
				break;
			case GL_OUT_OF_MEMORY:
				fprintf(stderr, "Out of memory.\n");
				break;
			}
			return;
		}
	}
#endif

	void
		GLSLProgram::SaveProgramBinary(const char* fileName, GLenum * format)
	{
		glProgramParameteri(this->Program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
		GLint length;
		glGetProgramiv(this->Program, GL_PROGRAM_BINARY_LENGTH, &length);
		GLubyte* buffer = new GLubyte[length];
		glGetProgramBinary(this->Program, length, NULL, format, buffer);

		fprintf(stderr, "Program binary format = 0x%04x\n", *format);

		FILE* fpout = fopen(fileName, "wb");
		if (fpout == NULL)
		{
			fprintf(stderr, "Cannot create output GLSL binary file '%s'\n", fileName);
			return;
		}
		fwrite(buffer, length, 1, fpout);
		fclose(fpout);
		delete[] buffer;
	}

	void
		GLSLProgram::LoadProgramBinary(const char* fileName, GLenum format)
	{
		FILE* fpin = fopen(fileName, "rb");
		if (fpin == NULL)
		{
			fprintf(stderr, "Cannot open input GLSL binary file '%s'\n", fileName);
			return;
		}
		fseek(fpin, 0, SEEK_END);
		GLint length = (GLint)ftell(fpin);
		GLubyte* buffer = new GLubyte[length];
		rewind(fpin);
		fread(buffer, length, 1, fpin);
		fclose(fpin);

		glProgramBinary(this->Program, format, buffer, length);
		delete[] buffer;

		GLint   success;
		glGetProgramiv(this->Program, GL_LINK_STATUS, &success);

		if (!success)
		{
			fprintf(stderr, "Did not successfully load the GLSL binary file '%s'\n", fileName);
			return;
		}
	}

	void
		GLSLProgram::SetGstap(bool b)
	{
		IncludeGstap = b;
	}

	GLchar* Gstap =
	{
		"#ifndef GSTAP_H\n\
#define GSTAP_H\n\
\n\
\n\
// gstap.h -- useful for glsl migration\n\
// from:\n\
//		Mike Bailey and Steve Cunningham\n\
//		\"Graphics Shaders: Theory and Practice\",\n\
//		Second Edition, AK Peters, 2011.\n\
\n\
\n\
\n\
// we are assuming that the compatibility #version line\n\
// is given in the source file, for example:\n\
// #version 400 compatibility\n\
\n\
\n\
// uniform variables:\n\
\n\
#define uModelViewMatrix		gl_ModelViewMatrix\n\
#define uProjectionMatrix		gl_ProjectionMatrix\n\
#define uModelViewProjectionMatrix	gl_ModelViewProjectionMatrix\n\
#define uNormalMatrix			gl_NormalMatrix\n\
#define uModelViewMatrixInverse		gl_ModelViewMatrixInverse\n\
\n\
// attribute variables:\n\
\n\
#define aColor				gl_Color\n\
#define aNormal				gl_Normal\n\
#define aVertex				gl_Vertex\n\
\n\
#define aTexCoord0			gl_MultiTexCoord0\n\
#define aTexCoord1			gl_MultiTexCoord1\n\
#define aTexCoord2			gl_MultiTexCoord2\n\
#define aTexCoord3			gl_MultiTexCoord3\n\
#define aTexCoord4			gl_MultiTexCoord4\n\
#define aTexCoord5			gl_MultiTexCoord5\n\
#define aTexCoord6			gl_MultiTexCoord6\n\
#define aTexCoord7			gl_MultiTexCoord7\n\
\n\
\n\
#endif		// #ifndef GSTAP_H\n\
\n\
\n"
	};

	inline float
		Sign(float x)
	{
		if (x < 0.)
			return -1.;
		return 1.;
	}

	// main program:
	int
		main(int argc, char* argv[])
	{
		// turn on the glut package:
		// (do this before checking argc and argv since it might
		// pull some command line arguments out)

		glutInit(&argc, argv);


		// setup all the graphics stuff:

		InitGraphics();

		// create the display structures that will not change:

		InitList();
		InitLists();
		MakeSphere(radius, slices, stacks);

		// init all the global variables used by Display( ):
		// this will also post a redisplay

		Reset();
		//InitGlui();

		// setup all the user interface stuff:

		//InitMenus( );


		// draw the scene once and wait for some interaction:
		// (this will never return)

		glutSetWindow(MainWindow);
		glutMainLoop();


		// this is here to make the compiler happy:

		return 0;
	}


	// this is where one would put code that is to be called
	// everytime the glut main loop has nothing to do
	//
	// this is typically where animation parameters are set
	//
	// do not call Display( ) from here -- let glutMainLoop( ) do it

	void Animate()
	{
		// put animation stuff in here -- change some global variables
		// for Display( ) to find:

		// force a call to Display( ) next time it is convenient:
		int ms = glutGet(GLUT_ELAPSED_TIME);
		ms %= MS_PER_CYCLE;
		Time = (float)ms / (float)MS_PER_CYCLE;		// [0.,1.)
		Distort += 2.0;
		if (Distort > 400) {
			Distort = 0;
		}

		if (uNoisen == true) {
			int s = glutGet(GLUT_ELAPSED_TIME);
			s /= S_PER_CYCLE;
			myTime = (float)s / 1000;

			if (myTime > 0.05 || myTime < 0) {
				myTime = 0;
			}
		}
		else {
			myTime = 0;
		}

		if (uNoisefn == true) {
			int s1 = glutGet(GLUT_ELAPSED_TIME);
			s1 /= S_PER_CYCLE;
			myTime1 = (float)s1 / 10;

			if (myTime1 > 10.0 || myTime1 < 0.0) {
				myTime1 = 0.f;
			}
		}
		else {
			myTime1 = 0;
		}

		if (uAn == true) {
			int s2 = glutGet(GLUT_ELAPSED_TIME);
			s2 /= S_PER_CYCLE;
			myTime2 = (float)s2 / 1000;
			printf("myTime is: %f", myTime2);
			if (myTime2 > 0.5 || myTime2 < 0.0) {
				myTime2 = 0.f;
			}
		}
		else {
			myTime2 = 0.f;
		}

		if (uBn == true) {
			int s3 = glutGet(GLUT_ELAPSED_TIME);
			s3 /= S_PER_CYCLE;
			myTime3 = (float)s3 / 1000;
			printf("myTime is: %f", myTime3);
			if (myTime3 > 0.5 || myTime3 < 0.0) {
				myTime3 = 0.f;
			}
		}
		else {
			myTime3 = 0.f;
		}

		int i;				// counter	
		struct fire* p;
		float dt;

		LavaTime += Dt;
		if (LavaTime > TMAX)
		{
			LavaTime = 0.;
		}

		// ****************************************
		// Here is where you advance your particles to reflect the current Time:
		// ****************************************
		for (i = 0, p = Fire; i < NUMFIRES; i++, p++)
		{
			dt = LavaTime - p->t0;
			p->x = (p->vx * dt) + p->x0;
			p->y = ((p->vy * dt) + 0.5 * GRAVITY * (dt * dt)) + p->y0;
			p->z = (p->vz * dt) + p->z0;

		}

		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}

	void
		Buttons(int id)
	{
		switch (id)
		{
		case RESET:
			Reset();
			//Glui->sync_live();
			glutSetWindow(MainWindow);
			glutPostRedisplay();
			break;

		case QUIT:
			Glui->close();
			glutSetWindow(MainWindow);
			glFinish();
			glutDestroyWindow(MainWindow);
			exit(0);
			break;

		default:
			fprintf(stderr, "Don't know what to do with Button ID %d\n", id);
		}

	}

	// draw the complete scene:

#define TOP	2147483647.		// 2^31 - 1	

	float
		Ranf(float low, float high)
	{
		long random();		// returns integer 0 - TOP
		float r;		// random number	

		r = (float)rand();

		return(low + r * (high - low) / (float)RAND_MAX);
	}

	void
		Display()
	{
		GLfloat scale2;		// real glui scale factor
		int i;			// counter		
		int j;
		int ti;
		struct fire* p;

		float dx = BOXSIZE / 2.f;
		float dy = BOXSIZE / 2.f;
		float dz = BOXSIZE / 2.f;
		if (DebugOn != 0)
		{
			fprintf(stderr, "Display\n");
		}


		// set which window we want to do the graphics into:

		glutSetWindow(MainWindow);

		Animate();
		// erase the background:

		glDrawBuffer(GL_BACK);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (DepthBufferOn != 0)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);

		// specify shading to be flat:

		glShadeModel(GL_FLAT);


		// set the viewport to a square centered in the window:

		GLsizei vx = glutGet(GLUT_WINDOW_WIDTH);
		GLsizei vy = glutGet(GLUT_WINDOW_HEIGHT);
		GLsizei v = vx < vy ? vx : vy;			// minimum dimension
		GLint xl = (vx - v) / 2;
		GLint yb = (vy - v) / 2;
		glViewport(xl, yb, v, v);


		// set the viewing volume:
		// remember that the Z clipping  values are actually
		// given as DISTANCES IN FRONT OF THE EYE
		// USE gluOrtho2D( ) IF YOU ARE DOING 2D !

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		if (WhichProjection == ORTHO)
			glOrtho(-3., 3., -3., 3., 0.1, 1000.);
		else
			gluPerspective(90., 1., 0.1, 1000.);


		// place the objects into the scene:

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// set the eye position, look-at position, and up-vector:

		gluLookAt(-3., 0., 5., 0., 0., 0., 0., 1., 0.);


		// rotate the scene:

		glRotatef((GLfloat)Yrot, 0., 1., 0.);
		glRotatef((GLfloat)Xrot, 1., 0., 0.);


		// uniformly scale the scene:

		if (Scale < MINSCALE)
			Scale = MINSCALE;
		glScalef((GLfloat)Scale, (GLfloat)Scale, (GLfloat)Scale);

		// set the fog parameters:

		if (DepthCueOn != 0)
		{
			glFogi(GL_FOG_MODE, FOGMODE);
			glFogfv(GL_FOG_COLOR, FOGCOLOR);
			glFogf(GL_FOG_DENSITY, FOGDENSITY);
			glFogf(GL_FOG_START, FOGSTART);
			glFogf(GL_FOG_END, FOGEND);
			glEnable(GL_FOG);
		}
		else
		{
			glDisable(GL_FOG);
		}


		// possibly draw the axes:

		if (AxesOn != 0)
		{
			glColor3fv(&Colors[WhichColor][0]);
			glCallList(AxesList);
		}


		// since we are using glScalef( ), be sure normals get unitized:

		glEnable(GL_NORMALIZE);
		if (Distort) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, tex1);
			//glRotatef(Time * 300, 1, -1, -1);
			glScalef(3, 3, 3);
			glBegin(GL_QUADS);
			dx = BOXSIZE / 2.f + Time / 40;
			dy = BOXSIZE / 2.f + Time / 40;
			dz = BOXSIZE / 2.f + Time / 100;
			int s = 1 + Time * 40;
			int t = 1 + Time * 40;
			//glScalef(5.f, 5.f, 5.f);
			glColor3f(0., 0., 1.);//1
			glNormal3f(0., 0., 1.);
			glTexCoord2f(0, 0);
			glVertex3f(-dx, -dy, dz);
			glTexCoord2f(0, dy);
			glVertex3f(dx, -dy, dz);
			glTexCoord2f(dx, dy);
			glVertex3f(dx, dy, dz);
			glTexCoord2f(dx, 0);
			glVertex3f(-dx, dy, dz);

			glNormal3f(0., 0., -1.);//2
			glTexCoord2f(0., 0.);
			glVertex3f(-dx, -dy, -dz);
			glTexCoord2f(0., dy);
			glVertex3f(-dx, dy, -dz);
			glTexCoord2f(dx, dy);
			glVertex3f(dx, dy, -dz);
			glTexCoord2f(dx, 0);
			glVertex3f(dx, -dy, -dz);

			glColor3f(1., 0., 0.);//3
			glNormal3f(1., 0., 0.);
			glTexCoord2f(0, 0);
			glVertex3f(dx, -dy, dz);
			glTexCoord2f(0, dy);
			glVertex3f(dx, -dy, -dz);
			glTexCoord2f(dx, dy);
			glVertex3f(dx, dy, -dz);
			glTexCoord2f(dx, 0);
			glVertex3f(dx, dy, dz);

			glNormal3f(-1., 0., 0.);//4
			glTexCoord2f(0, 0);
			glVertex3f(-dx, -dy, dz);
			glTexCoord2f(0, dy);
			glVertex3f(-dx, dy, dz);
			glTexCoord2f(dx, dy);
			glVertex3f(-dx, dy, -dz);
			glTexCoord2f(dx, 0);
			glVertex3f(-dx, -dy, -dz);

			glColor3f(0., 1., 0.);//5
			glNormal3f(0., 1., 0.);
			glTexCoord2f(0, 0);
			glVertex3f(-dx, dy, dz);
			glTexCoord2f(0, dy);
			glVertex3f(dx, dy, dz);
			glTexCoord2f(dx, dy);
			glVertex3f(dx, dy, -dz);
			glTexCoord2f(dx, 0);
			glVertex3f(-dx, dy, -dz);

			glNormal3f(0., -1., 0.);//6
			glTexCoord2f(0, 0);
			glVertex3f(-dx, -dy, dz);
			glTexCoord2f(0, dy);
			glVertex3f(-dx, -dy, -dz);
			glTexCoord2f(dx, dy);
			glVertex3f(dx, -dy, -dz);
			glTexCoord2f(dx, 0);
			glVertex3f(dx, -dy, dz);

			glEnd();
			Animate();
			Distort = 0;
		}
		else {
			dx = 25.f;
			dy = 25.f;
			dz = 25.f;
			glScalef(0, 0, 0);
			glScalef(5.f, 5.f, 5.f);
			glColor3f(0., 0., 1.);//1
			glNormal3f(0., 0., 1.);
			glTexCoord2f(0, 0);
			glVertex3f(-dx, -dy, dz);
			glTexCoord2f(0, 1);
			glVertex3f(dx, -dy, dz);
			glTexCoord2f(1, 1);
			glVertex3f(dx, dy, dz);
			glTexCoord2f(1, 0);
			glVertex3f(-dx, dy, dz);

			glNormal3f(0., 0., -1.);//2
			glTexCoord2f(0., 0.);
			glVertex3f(-dx, -dy, -dz);
			glTexCoord2f(0., 1.);
			glVertex3f(-dx, dy, -dz);
			glTexCoord2f(1., 1.);
			glVertex3f(dx, dy, -dz);
			glTexCoord2f(1., 0.);
			glVertex3f(dx, -dy, -dz);

			glColor3f(1., 0., 0.);//3
			glNormal3f(1., 0., 0.);
			glTexCoord2f(0, 0);
			glVertex3f(dx, -dy, dz);
			glTexCoord2f(0, 1);
			glVertex3f(dx, -dy, -dz);
			glTexCoord2f(1, 1);
			glVertex3f(dx, dy, -dz);
			glTexCoord2f(1, 0);
			glVertex3f(dx, dy, dz);

			glNormal3f(-1., 0., 0.);//4
			glTexCoord2f(0, 0);
			glVertex3f(-dx, -dy, dz);
			glTexCoord2f(0, 1);
			glVertex3f(-dx, dy, dz);
			glTexCoord2f(1, 1);
			glVertex3f(-dx, dy, -dz);
			glTexCoord2f(1, 0);
			glVertex3f(-dx, -dy, -dz);

			glColor3f(0., 1., 0.);//5
			glNormal3f(0., 1., 0.);
			glTexCoord2f(0, 0);
			glVertex3f(-dx, dy, dz);
			glTexCoord2f(0, 1);
			glVertex3f(dx, dy, dz);
			glTexCoord2f(1, 1);
			glVertex3f(dx, dy, -dz);
			glTexCoord2f(1, 0);
			glVertex3f(-dx, dy, -dz);

			glNormal3f(0., -1., 0.);//6
			glTexCoord2f(0, 0);
			glVertex3f(-dx, -dy, dz);
			glTexCoord2f(0, 1);
			glVertex3f(-dx, -dy, -dz);
			glTexCoord2f(1, 1);
			glVertex3f(dx, -dy, -dz);
			glTexCoord2f(1, 0);
			glVertex3f(dx, -dy, dz);

			glEnd();
			Animate();
		}
		Pattern->Use();
		

		uAA = 0.05 + myTime2;
		uPP = 0.05 + myTime3;
		uTol = 0;
		uNoiseAmp = myTime;
		uNoiseFreq = myTime1;
		alpha = 1.0;
		ublue = -3.8;
		ured = -1.1;


		Pattern->SetUniformVariable("uAd", uAA);
		Pattern->SetUniformVariable("uBd", uPP);
		Pattern->SetUniformVariable("uNoiseAmp", uNoiseAmp);
		Pattern->SetUniformVariable("uNoiseFreq", uNoiseFreq);
		Pattern->SetUniformVariable("uTol", uTol);

		Pattern->SetUniformVariable("uAlpha", alpha);
		Pattern->SetUniformVariable("uChromaBlue", ublue);
		Pattern->SetUniformVariable("uChromaRed", ured);
		Pattern->SetUniformVariable("uUseChromaDepth", uUseChromadepth);
		glScalef(0.01, 0.01, 0.01);
		glCallList(SphereList);

		/*
		AA = 0.3;
		BB = 0.8;
		CC = 0.1;
		DD = 0.1;
		Pattern->SetUniformVariable("uA", AA);
		Pattern->SetUniformVariable("uB", BB);
		Pattern->SetUniformVariable("uC", CC);
		Pattern->SetUniformVariable("uD", DD);
		*/
		
		//glCallList(BoxList);
		//Pattern->SetUniformVariable("uColor", glColor4f(1., 0.7, 1,1));

		Pattern->Use(0);

		p = Fire;
		float r = Ranf(RMIN, RMAX);
		glPointSize(r);
		p->x = Ranf(0, XMAX);
		p->y = sin(Ranf(0, YMAX));
		p->z = cos(Ranf(0, ZMAX));
		float r1 = 0.1;
		float g2 = 0;
		float b3 = 0;
		glEnable(GL_POINT_SMOOTH);
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
		glBegin(GL_POINTS);

		for (i = 0; i < NumFires; i++, p++)
		{
			if (p->t0 <= Time && Time <= p->t1)
			{
				glColor3f(p->r, p->g, p->b);
				glVertex3f(p->x, p->y, p->z);
			}
		}
		p->x = Ranf(0, XMAX);
		p->y = sin(Ranf(0, YMAX));
		p->z = cos(Ranf(0, ZMAX));
		glEnd();
		glDisable(GL_POINT_SMOOTH);
		glPointSize(1.);

		if (DepthFightingOn != 0)
		{
			glPushMatrix();
			glRotatef(90., 0., 1., 0.);
			glCallList(BoxList);
			glPopMatrix();
		}



		// draw some gratuitous text that just rotates on top of the scene:

		glDisable(GL_DEPTH_TEST);
		glColor3f(0., 1., 1.);
		DoRasterString(0., 1., 0., "");


		// draw some gratuitous text that is fixed on the screen:
		//
		// the projection matrix is reset to define a scene whose
		// world coordinate system goes from 0-100 in each axis
		//
		// this is called "percent units", and is just a convenience
		//
		// the modelview matrix is reset to identity as we don't
		// want to transform these coordinates

		glDisable(GL_DEPTH_TEST);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0., 100., 0., 100.);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glColor3f(1., 1., 1.);
		DoRasterString(5., 5., 0., "");

		// swap the double-buffered framebuffers:

		glutSwapBuffers();


		// be sure the graphics buffer has been sent:
		// note: be sure to use glFlush( ) here, not glFinish( ) !

		glFlush();
	}

	void
		DoAxesMenu(int id)
	{
		AxesOn = id;

		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}

	void
		DoColorMenu(int id)
	{
		WhichColor = id - RED;

		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}

	void
		DoDebugMenu(int id)
	{
		DebugOn = id;

		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}

	void
		DoDepthBufferMenu(int id)
	{
		DepthBufferOn = id;

		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}

	void
		DoDepthFightingMenu(int id)
	{
		DepthFightingOn = id;

		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}

	void
		DoDepthMenu(int id)
	{
		DepthCueOn = id;

		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}

	// main menu callback:

	void
		DoMainMenu(int id)
	{
		switch (id)
		{
		case RESET:
			Reset();
			break;

		case QUIT:
			// gracefully close out the graphics:
			// gracefully close the graphics window:
			// gracefully exit the program:
			glutSetWindow(MainWindow);
			glFinish();
			glutDestroyWindow(MainWindow);
			exit(0);
			break;

		default:
			fprintf(stderr, "Don't know what to do with Main Menu ID %d\n", id);
		}

		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}
	void
		DoProjectMenu(int id)
	{
		WhichProjection = id;

		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}

	// use glut to display a string of characters using a raster font:
	void
		DoRasterString(float x, float y, float z, char* s)
	{
		glRasterPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z);

		char c;			// one character to print
		for (; (c = *s) != '\0'; s++)
		{
			glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
		}
	}
	// use glut to display a string of characters using a stroke font:
	void
		DoStrokeString(float x, float y, float z, float ht, char* s)
	{
		glPushMatrix();
		glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
		float sf = ht / (119.05f + 33.33f);
		glScalef((GLfloat)sf, (GLfloat)sf, (GLfloat)sf);
		char c;			// one character to print
		for (; (c = *s) != '\0'; s++)
		{
			glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
		}
		glPopMatrix();
	}
	// return the number of seconds since the start of the program:
	float
		ElapsedSeconds()
	{
		// get # of milliseconds since the start of the program:

		int ms = glutGet(GLUT_ELAPSED_TIME);

		// convert it to seconds:

		return (float)ms / 1000.f;
	}

	void myGlutIdle(void) {
		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}



	void
		InitGraphics()
	{
		// request the display modes:
		// ask for red-green-blue-alpha color, double-buffering, and z-buffering:

		glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

		// set the initial window configuration:

		glutInitWindowPosition(0, 0);
		glutInitWindowSize(INIT_WINDOW_SIZE, INIT_WINDOW_SIZE);

		// open the window and set its title:

		MainWindow = glutCreateWindow(WINDOWTITLE);
		glutSetWindowTitle(WINDOWTITLE);

		// set the framebuffer clear values:

		glClearColor(BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3]);

		// setup the callback functions:
		// DisplayFunc -- redraw the window
		// ReshapeFunc -- handle the user resizing the window
		// KeyboardFunc -- handle a keyboard input
		// MouseFunc -- handle the mouse button going down or up
		// MotionFunc -- handle the mouse moving with a button down
		// PassiveMotionFunc -- handle the mouse moving with a button up
		// VisibilityFunc -- handle a change in window visibility
		// EntryFunc	-- handle the cursor entering or leaving the window
		// SpecialFunc -- handle special keys on the keyboard
		// SpaceballMotionFunc -- handle spaceball translation
		// SpaceballRotateFunc -- handle spaceball rotation
		// SpaceballButtonFunc -- handle spaceball button hits
		// ButtonBoxFunc -- handle button box hits
		// DialsFunc -- handle dial rotations
		// TabletMotionFunc -- handle digitizing tablet motion
		// TabletButtonFunc -- handle digitizing tablet button hits
		// MenuStateFunc -- declare when a pop-up menu is in use
		// TimerFunc -- trigger something to happen a certain time from now
		// IdleFunc -- what to do when nothing else is going on

		glutSetWindow(MainWindow);
		glutDisplayFunc(Display);
		glutReshapeFunc(Resize);
		glutKeyboardFunc(Keyboard);
		glutMouseFunc(MouseButton);
		glutMotionFunc(MouseMotion);
		glutPassiveMotionFunc(NULL);
		glutVisibilityFunc(Visibility);
		glutEntryFunc(NULL);
		glutSpecialFunc(NULL);
		glutSpaceballMotionFunc(NULL);
		glutSpaceballRotateFunc(NULL);
		glutSpaceballButtonFunc(NULL);
		glutButtonBoxFunc(NULL);
		glutDialsFunc(NULL);
		glutTabletMotionFunc(NULL);
		glutTabletButtonFunc(NULL);
		glutMenuStateFunc(NULL);
		glutTimerFunc(-1, NULL, 0);
		glutIdleFunc(Animate);

		Fire = new struct fire[NUMFIRES];
		int i;
		struct fire* p;
		for (i = 0, p = Fire; i < NUMFIRES; i++, p++)
		{
			float v = Ranf(VELMIN, VELMAX);
			float th = Ranf(THETAMIN, THETAMAX);
			float th2 = Ranf(0.f, 2.f * M_PI);
			double x_rot_y = (cos(th2) + sin(th2));
			double z_rot_y = (cos(th2) - sin(th2));

			p->vx0 = (v * cos(th));
			p->vx0 = p->vx0 > 0 ? p->vx0 : -p->vx0;
			p->vx0 *= x_rot_y;

			p->vz0 = (v * cos(th));
			p->vz0 = p->vz0 > 0 ? p->vz0 : -p->vz0;
			p->vz0 *= z_rot_y;

			p->vy0 = v * sin(th);

			float r = Ranf(RMIN, RMAX);
			p->x0 = r * x_rot_y;
			p->z0 = r * z_rot_y;
			p->y0 = 0.f;

			p->t0 = Ranf(TMIN, TMAX / 2.0f);
			p->t1 = Ranf(TMIN / 2.0f, TMAX);

			p->r = 1.f;
			p->g = 0.f;
			p->b = 0.f;
			p->ti = 5;
			for (int j = 0; j < NUMTRACES; j++)
			{
			}
		}

#ifdef WIN32
		GLenum err = glewInit();
		if (err != GLEW_OK)
		{
			fprintf(stderr, "glewInit Error\n");
		}
		else
			fprintf(stderr, "GLEW initialized OK\n");
		fprintf(stderr, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif

		Texture1 = BmpToTexture("Lava2.bmp", &width, &height);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glGenTextures(1, &tex1);
		glBindTexture(GL_TEXTURE_2D, tex1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, 1024, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, Texture1);

		Pattern = new GLSLProgram();
		//Pattern2 = new GLSLProgram();
		bool valid = Pattern->Create("pattern.vert", "pattern.frag");
		//bool valid = Pattern->Create("rock.vert", "rock.frag");
		if (!valid)
		{
			fprintf(stderr, "Shader cannot be created!\n");
			DoMainMenu(QUIT);
		}
		else
		{
			fprintf(stderr, "Shader created.\n");
		}
		Pattern->SetVerbose(false);
		//Pattern2->SetVerbose(false);
		// init glew (a window must be open to do this):



	}


	int
		ReadInt(FILE * fp)
	{
		unsigned char b3, b2, b1, b0;
		b0 = fgetc(fp);
		b1 = fgetc(fp);
		b2 = fgetc(fp);
		b3 = fgetc(fp);
		return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
	}


	short
		ReadShort(FILE * fp)
	{
		unsigned char b1, b0;
		b0 = fgetc(fp);
		b1 = fgetc(fp);
		return (b1 << 8) | b0;
	}
	// initialize the display lists that will not change:
	// (a display list is a way to store opengl commands in
	//  memory so that they can be played back efficiently at a later time
	//  with a call to glCallList( )

	void
		InitLists()
	{
		float dx = 10.f;
		float dy = 10.f;
		float dz = 10.f;
		glutSetWindow(MainWindow);

		// create the object:

		BoxList = glGenLists(1);
		glNewList(BoxList, GL_COMPILE);

		glBegin(GL_QUADS);
		glNormal3f(0., -1., 0.);
		glVertex3f(-dx, -dy, dz);
		glVertex3f(-dx, -dy, -dz);
		glVertex3f(dx, -dy, -dz);
		glVertex3f(dx, -dy, dz);

		glEnd();

		glEndList();


		// create the axes:

		AxesList = glGenLists(1);
		glNewList(AxesList, GL_COMPILE);
		glLineWidth(AXES_WIDTH);
		Axes(1.5);
		glLineWidth(1.);
		glEndList();
	}

	void
		InitList(void)
	{
		if (Debug)
			fprintf(stderr, "InitLists\n");

		ParticleList = glGenLists(1);
		glNewList(ParticleList, GL_COMPILE);
		glutSolidSphere(1., 8, 8);
		glEndList();

		AxesList = glGenLists(1);
		glNewList(AxesList, GL_COMPILE);
		glColor3fv(AXES_COLOR);
		glLineWidth(AXES_WIDTH);
		Axes(100.);
		glLineWidth(1.);
		glEndList();
	}
	void
		MakeSphere(float radius, int slices, int stacks) {
		glutSetWindow(MainWindow);
		SphereList = glGenLists(1);
		glNewList(SphereList, GL_COMPILE);

		struct point top, bot;		// top, bottom points
		struct point* p;

		// set the globals:

		NumLngs = slices;
		NumLats = stacks;

		if (NumLngs < 3)
			NumLngs = 3;

		if (NumLats < 3)
			NumLats = 3;


		// allocate the point data structure:

		Pts = new struct point[NumLngs * NumLats];


		// fill the Pts structure:

		for (int ilat = 0; ilat < NumLats; ilat++)
		{
			float lat = -M_PI / 2. + M_PI * (float)ilat / (float)(NumLats - 1);
			float xz = cos(lat);
			float y = sin(lat);
			for (int ilng = 0; ilng < NumLngs; ilng++)
			{
				float lng = -M_PI + 2. * M_PI * (float)ilng / (float)(NumLngs - 1);
				float x = xz * cos(lng);
				float z = -xz * sin(lng);
				p = PtsPointer(ilat, ilng);
				p->x = radius * x;
				p->y = radius * y;
				p->z = radius * z;
				p->nx = x;
				p->ny = y;
				p->nz = z;
				p->s = (lng + M_PI) / (M_PI);
				p->t = (lat + M_PI) / M_PI;
			}
		}

		top.x = 0.;		top.y = radius;	top.z = 0.;
		top.nx = 0.;		top.ny = 1.;		top.nz = 0.;
		top.s = 0.;		top.t = 1.;

		bot.x = 0.;		bot.y = -radius;	bot.z = 0.;
		bot.nx = 0.;		bot.ny = -1.;		bot.nz = 0.;
		bot.s = 0.;		bot.t = -1.;


		// connect the north pole to the latitude NumLats-2:

		glBegin(GL_QUADS);
		for (int ilng = 0; ilng < NumLngs - 1; ilng++)
		{
			p = PtsPointer(NumLats - 1, ilng);
			DrawPoint(p);

			p = PtsPointer(NumLats - 2, ilng);
			DrawPoint(p);

			p = PtsPointer(NumLats - 2, ilng + 1);
			DrawPoint(p);

			p = PtsPointer(NumLats - 1, ilng + 1);
			DrawPoint(p);
		}
		glEnd();

		// connect the south pole to the latitude 1:

		glBegin(GL_QUADS);
		for (int ilng = 0; ilng < NumLngs - 1; ilng++)
		{
			p = PtsPointer(0, ilng);
			DrawPoint(p);

			p = PtsPointer(0, ilng + 1);
			DrawPoint(p);

			p = PtsPointer(1, ilng + 1);
			DrawPoint(p);

			p = PtsPointer(1, ilng);
			DrawPoint(p);
		}
		glEnd();


		// connect the other 4-sided polygons:

		glBegin(GL_QUADS);
		for (int ilat = 2; ilat < NumLats - 1; ilat++)
		{
			for (int ilng = 0; ilng < NumLngs - 1; ilng++)
			{
				p = PtsPointer(ilat - 1, ilng);
				DrawPoint(p);

				p = PtsPointer(ilat - 1, ilng + 1);
				DrawPoint(p);

				p = PtsPointer(ilat, ilng + 1);
				DrawPoint(p);

				p = PtsPointer(ilat, ilng);
				DrawPoint(p);
			}
		}
		glEnd();

		delete[] Pts;
		Pts = NULL;
		glEndList();

		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}



	// the keyboard callback:

	void
		Keyboard(unsigned char c, int x, int y)
	{
		if (DebugOn != 0)
			fprintf(stderr, "Keyboard: '%c' (0x%0x)\n", c, c);

		switch (c)
		{
		case 'o':
		case 'O':
			WhichProjection = ORTHO;
			break;

		case 'f':
			FreezeAnimation = !FreezeAnimation;
			glutIdleFunc(NULL);
			break;

		case 'A':
			uNoisen = !uNoisen;
			break;

		case 'F':
			uNoisefn = !uNoisefn;
			break;

		case 'C':
			uAn = !uAn;
			break;

		case 'L':
			GLUI_Master.set_glutIdleFunc(Animate);//PARTICLES
			break;

		case 'l':
			GLUI_Master.set_glutIdleFunc(NULL);
			break;

		case 'D':
			uBn = !uBn;
			break;

		case 'b':
			BothAnimation = !BothAnimation;
			glutIdleFunc(Animate);
			break;

		case 'p':
		case 'P':
			WhichProjection = PERSP;
			break;

		case 'q':
		case 'Q':
		case ESCAPE:
			DoMainMenu(QUIT);	// will not return here
			break;				// happy compiler

		default:
			fprintf(stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c);
		}

		//Glui->sync_live();
		// force a call to Display( ):

		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}


	// called when the mouse button transitions down or up:
	/////////////////////////////////////////////////////
	void
		MouseButton(int button, int state, int x, int y)
	{
		int b = 0;			// LEFT, MIDDLE, or RIGHT

		if (DebugOn != 0)
			fprintf(stderr, "MouseButton: %d, %d, %d, %d\n", button, state, x, y);


		// get the proper button bit mask:

		switch (button)
		{
		case GLUT_LEFT_BUTTON:
			b = LEFT;		break;



		case GLUT_MIDDLE_BUTTON:
			b = MIDDLE;		break;

		case GLUT_RIGHT_BUTTON:
			b = RIGHT;		break;

		default:
			b = 0;
			fprintf(stderr, "Unknown mouse button: %d\n", button);
		}


		// button down sets the bit, up clears the bit:

		if (state == GLUT_DOWN)
		{
			Xmouse = x;
			Ymouse = y;
			ActiveButton |= b;		// set the proper bit
		}
		else
		{
			ActiveButton &= ~b;		// clear the proper bit
		}
	}


	// called when the mouse moves while a button is down:

	void
		MouseMotion(int x, int y)
	{
		if (DebugOn != 0)
			fprintf(stderr, "MouseMotion: %d, %d\n", x, y);


		int dx = x - Xmouse;		// change in mouse coords
		int dy = y - Ymouse;

		if ((ActiveButton & LEFT) != 0)
		{
			Xrot += (ANGFACT * dy);
			Yrot += (ANGFACT * dx);
		}


		if ((ActiveButton & MIDDLE) != 0)
		{
			Scale += SCLFACT * (float)(dx - dy);

			// keep object from turning inside-out or disappearing:

			if (Scale < MINSCALE)
				Scale = MINSCALE;
		}

		Xmouse = x;			// new current position
		Ymouse = y;

		//Glui->sync_live();

		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}


	// reset the transformations and the colors:
	// this only sets the global variables --
	// the glut main loop is responsible for redrawing the scene

	void
		Reset()
	{
		ActiveButton = 0;
		AxesOn = GLUITRUE;
		Debug = GLUIFALSE;
		DepthCueOn = FALSE;
		DoFan = GLUIFALSE;
		DoTraces = GLUIFALSE;
		Dt = DT;
		LeftButton = ROTATE;
		NumFires = NUMFIRES;
		Scale = 1.0;
		Scale2 = 0.0;		// because add 1. to it in Display()
		WhichProjection = PERSP;
		Xrot = Yrot = 0.;
		TransXYZ[0] = TransXYZ[1] = TransXYZ[2] = 0.;

		RotMatrix[0][1] = RotMatrix[0][2] = RotMatrix[0][3] = 0.;
		RotMatrix[1][0] = RotMatrix[1][2] = RotMatrix[1][3] = 0.;
		RotMatrix[2][0] = RotMatrix[2][1] = RotMatrix[2][3] = 0.;
		RotMatrix[3][0] = RotMatrix[3][1] = RotMatrix[3][3] = 0.;
		RotMatrix[0][0] = RotMatrix[1][1] = RotMatrix[2][2] = RotMatrix[3][3] = 1.;


		// ****************************************
		// Here is where you reset your particle system data structures to the initial configuration:
		// ****************************************

		int i;
		int j;
		struct fire* p;
		for (i = 0, p = Fire; i < NUMFIRES; i++, p++)
		{
			p->x = p->x0;
			p->y = p->y0;
			p->z = p->z0;
			p->vx = p->vx0;
			p->vy = p->vy0;
			p->vz = p->vz0;
			if (DoTraces)
			{
				p->ti = 0;
				for (j = 0; j < NUMTRACES; j++)
				{
					p->tx[j] = p->x;
					p->ty[j] = p->y;
					p->tz[j] = p->z;
				}
			}
		}

		Time = TMIN;
	}

	// called when user resizes the window:
	void
		Resize(int width, int height)
	{
		if (DebugOn != 0)
			fprintf(stderr, "ReSize: %d, %d\n", width, height);

		// don't really need to do anything since window size is
		// checked each time in Display( ):

		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}

	// handle a change to the window's visibility:

	void
		Visibility(int state)
	{
		if (DebugOn != 0)
			fprintf(stderr, "Visibility: %d\n", state);

		if (state == GLUT_VISIBLE)
		{
			glutSetWindow(MainWindow);
			glutPostRedisplay();
		}
		else
		{
			// could optimize by keeping track of the fact
			// that the window is not visible and avoid
			// animating or redrawing it ...
		}
	}

	///////////////////////////////////////   HANDY UTILITIES:  //////////////////////////


	// the stroke characters 'X' 'Y' 'Z' :

	static float xx[] = {
			0.f, 1.f, 0.f, 1.f
	};

	static float xy[] = {
			-.5f, .5f, .5f, -.5f
	};

	static int xorder[] = {
			1, 2, -3, 4
	};

	static float yx[] = {
			0.f, 0.f, -.5f, .5f
	};

	static float yy[] = {
			0.f, .6f, 1.f, 1.f
	};

	static int yorder[] = {
			1, 2, 3, -2, 4
	};

	static float zx[] = {
			1.f, 0.f, 1.f, 0.f, .25f, .75f
	};

	static float zy[] = {
			.5f, .5f, -.5f, -.5f, 0.f, 0.f
	};

	static int zorder[] = {
			1, 2, 3, 4, -5, 6
	};

	// fraction of the length to use as height of the characters:
	const float LENFRAC = 0.10f;

	// fraction of length to use as start location of the characters:
	const float BASEFRAC = 1.10f;

	//	Draw a set of 3D axes:
	//	(length is the axis length in world coordinates)

	void
		Axes(float length)
	{
		glBegin(GL_LINE_STRIP);
		glVertex3f(length, 0., 0.);
		glVertex3f(0., 0., 0.);
		glVertex3f(0., length, 0.);
		glEnd();
		glBegin(GL_LINE_STRIP);
		glVertex3f(0., 0., 0.);
		glVertex3f(0., 0., length);
		glEnd();

		float fact = LENFRAC * length;
		float base = BASEFRAC * length;

		glBegin(GL_LINE_STRIP);
		for (int i = 0; i < 4; i++)
		{
			int j = xorder[i];
			if (j < 0)
			{

				glEnd();
				glBegin(GL_LINE_STRIP);
				j = -j;
			}
			j--;
			glVertex3f(base + fact * xx[j], fact * xy[j], 0.0);
		}
		glEnd();

		glBegin(GL_LINE_STRIP);
		for (int i = 0; i < 5; i++)
		{
			int j = yorder[i];
			if (j < 0)
			{

				glEnd();
				glBegin(GL_LINE_STRIP);
				j = -j;
			}
			j--;
			glVertex3f(fact * yx[j], base + fact * yy[j], 0.0);
		}
		glEnd();

		glBegin(GL_LINE_STRIP);
		for (int i = 0; i < 6; i++)
		{
			int j = zorder[i];
			if (j < 0)
			{

				glEnd();
				glBegin(GL_LINE_STRIP);
				j = -j;
			}
			j--;
			glVertex3f(0.0, fact * zy[j], base + fact * zx[j]);
		}
		glEnd();

	}


	// function to convert HSV to RGB
	// 0.  <=  s, v, r, g, b  <=  1.
	// 0.  <= h  <=  360.
	// when this returns, call:
	//		glColor3fv( rgb );

	void
		HsvRgb(float hsv[3], float rgb[3])
	{
		// guarantee valid input:

		float h = hsv[0] / 60.f;
		while (h >= 6.)	h -= 6.;
		while (h < 0.) 	h += 6.;

		float s = hsv[1];
		if (s < 0.)
			s = 0.;
		if (s > 1.)
			s = 1.;

		float v = hsv[2];
		if (v < 0.)
			v = 0.;
		if (v > 1.)
			v = 1.;

		// if sat==0, then is a gray:

		if (s == 0.0)
		{
			rgb[0] = rgb[1] = rgb[2] = v;
			return;
		}

		// get an rgb from the hue itself:

		float i = floor(h);
		float f = h - i;
		float p = v * (1.f - s);
		float q = v * (1.f - s * f);
		float t = v * (1.f - (s * (1.f - f)));

		float r, g, b;			// red, green, blue
		switch ((int)i)
		{
		case 0:
			r = v;	g = t;	b = p;
			break;

		case 1:
			r = q;	g = v;	b = p;
			break;

		case 2:
			r = p;	g = v;	b = t;
			break;

		case 3:
			r = p;	g = q;	b = v;
			break;

		case 4:
			r = t;	g = p;	b = v;
			break;

		case 5:
			r = v;	g = p;	b = q;
			break;
		}


		rgb[0] = r;
		rgb[1] = g;
		rgb[2] = b;
	}
