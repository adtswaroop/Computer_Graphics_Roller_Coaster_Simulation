/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster
  C++ starter code

  Student username: Aditi Swaroop
*/


#include "basicPipelineProgram.h"
#include "phongPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <math.h>

#include <glm/gtc/type_ptr.hpp>

#if defined(WIN32) || defined(_WIN32)
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#endif

#if defined(WIN32) || defined(_WIN32)
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
int screenshots = 0;
char windowTitle[512] = "CSCI 420 homework II";

//variables
OpenGLMatrix openGLMatrix;
BasicPipelineProgram objPipelineProg;
GLint program_ID;

PhongPipelineProgram phongPipelineProg;
GLint phong_program_ID;

float m[16], p[16], view[16], n[16];

GLint h_modelMatrix, h_projection, h_normal, h_lightDirection, h_texModelViewMatrix, h_texProjectionMatrix;

float s = 0.5, scale = 0.3f;
int counter = 0;

std::vector<glm::vec3> spline_point;
std::vector<glm::vec3> ltrack_vertices;
std::vector<GLfloat>   ltrack_texCoords;
std::vector<glm::vec3> ltrack_normals;
std::vector<glm::vec3> rtrack_vertices;
std::vector<GLfloat>   rtrack_texCoords;
std::vector<glm::vec3> rtrack_normals;
std::vector<glm::vec3> bars_vertices;
std::vector<GLfloat>   bars_texCoords;
std::vector<glm::vec3> ground_vertices;
std::vector<GLfloat>   ground_texCoords;
std::vector<glm::vec3> skybox_vertices;
std::vector<GLfloat>   skybox_texCoords;

std::vector<glm::vec3> tangent_vertices;
std::vector<glm::vec3> normal_vertices;
std::vector<glm::vec3> binormal_vertices;

//std::vector<glm::vec4> spline_colors;

GLuint	ltrackVAO,	ltrackVBO , 
		rtrackVAO,	rtrackVBO ,
		barsVAO,	barsVBO	  ,
		groundVAO,	groundVBO ,
		skyboxVAO,	skyboxVBO ;

GLuint  h_railTex, h_barTex , h_groundTex, h_skyTex; 

// represents one control point along the spline 
struct Point
{
	double x;
	double y;
	double z;
};



// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline
{
	int numControlPoints;
	Point * points;
};

// the spline array 
Spline * splines;
// total number of splines 
int numSplines;

int loadSplines(char * argv)
{
	char * cName = (char *)malloc(128 * sizeof(char));
	FILE * fileList;
	FILE * fileSpline;
	int iType, i = 0, j, iLength;

	// load the track file 
	fileList = fopen(argv, "r");
	if (fileList == NULL)
	{
		printf("can't open file\n");
		exit(1);
	}

	// stores the number of splines in a global variable 
	fscanf(fileList, "%d", &numSplines);

	splines = (Spline*)malloc(numSplines * sizeof(Spline));

	// reads through the spline files 
	for (j = 0; j < numSplines; j++)
	{
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL)
		{
			printf("can't open file\n");
			exit(1);
		}

		// gets length for spline file
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		// allocate memory for all the points
		splines[j].points = (Point *)malloc(iLength * sizeof(Point));
		splines[j].numControlPoints = iLength;

		// saves the data to the struct
		while (fscanf(fileSpline, "%lf %lf %lf",
			&splines[j].points[i].x,
			&splines[j].points[i].y,
			&splines[j].points[i].z) != EOF)
		{
			i++;
		}
	}

	free(cName);

	return 0;
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
	// read the texture image
	ImageIO img;
	ImageIO::fileFormatType imgFormat;
	ImageIO::errorType err = img.load(imageFilename, &imgFormat);

	if (err != ImageIO::OK)
	{
		printf("Loading texture from %s failed.\n", imageFilename);
		return -1;
	}

	// check that the number of bytes is a multiple of 4
	if (img.getWidth() * img.getBytesPerPixel() % 4)
	{
		printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
		return -1;
	}

	// allocate space for an array of pixels
	int width = img.getWidth();
	int height = img.getHeight();
	unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

	// fill the pixelsRGBA array with the image pixels
	memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
	for (int h = 0; h < height; h++)
		for (int w = 0; w < width; w++)
		{
			// assign some default byte values (for the case where img.getBytesPerPixel() < 4)
			pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
			pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
			pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
			pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

			// set the RGBA channels, based on the loaded image
			int numChannels = img.getBytesPerPixel();
			for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
				pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
		}

	// bind the texture
	glBindTexture(GL_TEXTURE_2D, textureHandle);

	// initialize the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

	// generate the mipmaps for this texture
	glGenerateMipmap(GL_TEXTURE_2D);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// query support for anisotropic texture filtering
	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	printf("Max available anisotropic samples: %f\n", fLargest);
	// set anisotropic texture filtering
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

	// query for any errors
	GLenum errCode = glGetError();
	if (errCode != 0)
	{
		printf("Texture initialization error. Error code: %d.\n", errCode);
		return -1;
	}

	// de-allocate the pixel array -- it is no longer needed
	delete[] pixelsRGBA;

	return 0;
}

void initSplineVertices()
{
	int flag = 1;
	float prev_vertice[3];
	float curr_vertice[3];

	float M[4][4] = {
						{-s, 2 - s,	s - 2, s},
						{2 * s,	s - 3, 3 - (2 * s),	-s},
						{-s, 0, s, 0 },
						{0, 1, 0, 0 }
	};

	float UM[4], tangentUM[4];
	GLfloat UMC[3], tangentUMC[3];

	cout << "inside gen vertices";

	for (int n = 0; n < numSplines; n++) {
		for (int i = 0; i <= splines[n].numControlPoints - 4; i++)
		{
			float C[4][3] = {
								{ splines[n].points[i].x ,    splines[n].points[i].y    ,splines[n].points[i].z },
								{ splines[n].points[i + 1].x ,splines[n].points[i + 1].y,splines[n].points[i + 1].z },
								{ splines[n].points[i + 2].x ,splines[n].points[i + 2].y,splines[n].points[i + 2].z },
								{ splines[n].points[i + 3].x ,splines[n].points[i + 3].y,splines[n].points[i + 3].z },

			};

			for (float u = 0.000; u <= 1.0; u += 0.006)
			{
				float Umatrix[4] = { u*u*u , u*u, u, 1 };

				//multiply U and M
				UM[0] = Umatrix[0] * M[0][0] + Umatrix[1] * M[1][0] + Umatrix[2] * M[2][0] + Umatrix[3] * M[3][0];
				UM[1] = Umatrix[0] * M[0][1] + Umatrix[1] * M[1][1] + Umatrix[2] * M[2][1] + Umatrix[3] * M[3][1];
				UM[2] = Umatrix[0] * M[0][2] + Umatrix[1] * M[1][2] + Umatrix[2] * M[2][2] + Umatrix[3] * M[3][2];
				UM[3] = Umatrix[0] * M[0][3] + Umatrix[1] * M[1][3] + Umatrix[2] * M[2][3] + Umatrix[3] * M[3][3];

				//multiply UM and C
				UMC[0] = UM[0] * C[0][0] + UM[1] * C[1][0] + UM[2] * C[2][0] + UM[3] * C[3][0];
				UMC[1] = UM[0] * C[0][1] + UM[1] * C[1][1] + UM[2] * C[2][1] + UM[3] * C[3][1];
				UMC[2] = UM[0] * C[0][2] + UM[1] * C[1][2] + UM[2] * C[2][2] + UM[3] * C[3][2];

				glm::vec3 point = glm::make_vec3(UMC);

				spline_point.push_back(point);
				//cout << point.x << "," << point.y << "," << point.z << endl;
				
				//float color[] = { 1.0, 0.0, 0.0, 1.0 };
				//glm::vec4 col = glm::make_vec4(color);
				//spline_colors.push_back(col);

			}
		}

		//compute tangents along all spline points
		for (int j = 0; j <= spline_point.size() - 4; j++)
		{
			glm::vec3 p1 = spline_point[j];
			glm::vec3 p2 = spline_point[j + 1];
			glm::vec3 p3 = spline_point[j + 2];
			glm::vec3 p4 = spline_point[j + 3];

			float controlMat[4][3] = {
				p1.x, p1.y, p1.z,
				p2.x, p2.y, p2.z,
				p3.x, p3.y, p3.z,
				p4.x, p4.y, p4.z
			};

			float U[4] = { 3*0.01*0.01, 2*0.01 , 1, 0 };

			tangentUM[0] = U[0] * M[0][0] + U[1] * M[1][0] + U[2] * M[2][0] + U[3] * M[3][0];
			tangentUM[1] = U[0] * M[0][1] + U[1] * M[1][1] + U[2] * M[2][1] + U[3] * M[3][1];
			tangentUM[2] = U[0] * M[0][2] + U[1] * M[1][2] + U[2] * M[2][2] + U[3] * M[3][2];
			tangentUM[3] = U[0] * M[0][3] + U[1] * M[1][3] + U[2] * M[2][3] + U[3] * M[3][3];

			tangentUMC[0] = tangentUM[0] * controlMat[0][0] + tangentUM[1] * controlMat[1][0] + tangentUM[2] * controlMat[2][0] + tangentUM[3] * controlMat[3][0];
			tangentUMC[1] = tangentUM[0] * controlMat[0][1] + tangentUM[1] * controlMat[1][1] + tangentUM[2] * controlMat[2][1] + tangentUM[3] * controlMat[3][1];
			tangentUMC[2] = tangentUM[0] * controlMat[0][2] + tangentUM[1] * controlMat[1][2] + tangentUM[2] * controlMat[2][2] + tangentUM[3] * controlMat[3][2];

			glm::vec3 tPoint = glm::make_vec3(tangentUMC);

			//normalise tangent 
			glm::vec3 normalizedTangent = glm::normalize(tPoint);
			
			tangent_vertices.push_back(normalizedTangent);

			//implementing Sloan's method for camera movement
			//if it is the first point
			if (j == 0) {
				//arbitary vector
				float vec[3] = {1.0, 1.0, 1.0};
				glm::vec3 arbVector = glm::make_vec3(vec);
				
				glm::vec3 N0 = glm::cross(normalizedTangent, arbVector);
				glm::vec3 normalizedN0 = glm::normalize(N0);
				normal_vertices.push_back(normalizedN0);

				glm::vec3 B0 = glm::cross(normalizedTangent, normalizedN0);
				glm::vec3 normalizedB0 = glm::normalize(B0);
				binormal_vertices.push_back(normalizedB0);
			}
			else {
				glm::vec3 nextN = glm::cross(binormal_vertices[j - 1], normalizedTangent);
				glm::vec3 normalizedNextN = glm::normalize(nextN);
				normal_vertices.push_back(normalizedNextN);
				
				glm::vec3 nextB = glm::cross(normalizedTangent, normalizedNextN);
				glm::vec3 normalizedNextB = glm::normalize(nextB);
				binormal_vertices.push_back(normalizedNextB);
			}

		}
	}
	cout << "leaving gen vertices";
}

void initTrackVertices()
{
	cout << "inside initTrack" <<endl ;
	for (int n = 0; n < spline_point.size()- 5; n++) {

		glm::vec3 P0 = spline_point[n];
		glm::vec3 P1 = spline_point[n + 1];

#pragma region leftside
		//P0 points
		glm::vec3 V0 = {P0.x + scale * binormal_vertices[n].x + 0.05f * (-normal_vertices[n].x + binormal_vertices[n].x),
						P0.y + scale * binormal_vertices[n].y + 0.05f * (-normal_vertices[n].y + binormal_vertices[n].y),
						P0.z + scale * binormal_vertices[n].z + 0.05f * (-normal_vertices[n].z + binormal_vertices[n].z) };
							   
		glm::vec3 V1 = {P0.x + scale * binormal_vertices[n].x + 0.05f * (normal_vertices[n].x + binormal_vertices[n].x),
						P0.y + scale * binormal_vertices[n].y + 0.05f * (normal_vertices[n].y + binormal_vertices[n].y),
						P0.z + scale * binormal_vertices[n].z + 0.05f * (normal_vertices[n].z + binormal_vertices[n].z) };
							   
		glm::vec3 V2 = {P0.x + scale * binormal_vertices[n].x + 0.05f * (normal_vertices[n].x - binormal_vertices[n].x),
						P0.y + scale * binormal_vertices[n].y + 0.05f * (normal_vertices[n].y - binormal_vertices[n].y),
						P0.z + scale * binormal_vertices[n].z + 0.05f * (normal_vertices[n].z - binormal_vertices[n].z) };
							   
		glm::vec3 V3 = {P0.x + scale * binormal_vertices[n].x + 0.05f * (-normal_vertices[n].x + binormal_vertices[n].x),
						P0.y + scale * binormal_vertices[n].y + 0.05f * (-normal_vertices[n].y + binormal_vertices[n].y),
						P0.z + scale * binormal_vertices[n].z + 0.05f * (-normal_vertices[n].z + binormal_vertices[n].z) };

		//P1 points
		glm::vec3 V4 = {P1.x + scale * binormal_vertices[n+1].x + 0.05f * (-normal_vertices[n+1].x + binormal_vertices[n+1].x),
						P1.y + scale * binormal_vertices[n+1].y + 0.05f * (-normal_vertices[n+1].y + binormal_vertices[n+1].y),
						P1.z + scale * binormal_vertices[n+1].z + 0.05f * (-normal_vertices[n+1].z + binormal_vertices[n+1].z) };

		glm::vec3 V5 = {P1.x + scale * binormal_vertices[n+1].x + 0.05f * (normal_vertices[n+1].x + binormal_vertices[n+1].x),
						P1.y + scale * binormal_vertices[n+1].y + 0.05f * (normal_vertices[n+1].y + binormal_vertices[n+1].y),
						P1.z + scale * binormal_vertices[n+1].z + 0.05f * (normal_vertices[n+1].z + binormal_vertices[n+1].z) };

		glm::vec3 V6 = {P1.x + scale * binormal_vertices[n+1].x + 0.05f * (normal_vertices[n+1].x - binormal_vertices[n+1].x),
						P1.y + scale * binormal_vertices[n+1].y + 0.05f * (normal_vertices[n+1].y - binormal_vertices[n+1].y),
						P1.z + scale * binormal_vertices[n+1].z + 0.05f * (normal_vertices[n+1].z - binormal_vertices[n+1].z) };

		glm::vec3 V7 = {P1.x + scale * binormal_vertices[n+1].x + 0.05f * (-normal_vertices[n+1].x + binormal_vertices[n+1].x),
						P1.y + scale * binormal_vertices[n+1].y + 0.05f * (-normal_vertices[n+1].y + binormal_vertices[n+1].y),
						P1.z + scale * binormal_vertices[n+1].z + 0.05f * (-normal_vertices[n+1].z + binormal_vertices[n+1].z) };

		//texture coordinates
		glm::vec2 texCoord0 = { 1.0f, 0.0f };
		glm::vec2 texCoord1 = { 1.0f, 1.0f };
		glm::vec2 texCoord2 = { 0.0f, 1.0f };
		glm::vec2 texCoord3 = { 0.0f, 0.0f };

		//populate vertices for P0 as two triangles
		/*  V2-----V1
			|  \    |
			|    \  |
			V3-----V0
		
		*/

		ltrack_vertices.push_back(V3);
		ltrack_vertices.push_back(V0);
		ltrack_vertices.push_back(V2);

		ltrack_normals.push_back(tangent_vertices[n]);
		ltrack_normals.push_back(tangent_vertices[n]);
		ltrack_normals.push_back(tangent_vertices[n]);

		ltrack_vertices.push_back(V2);
		ltrack_vertices.push_back(V0);
		ltrack_vertices.push_back(V1);

		ltrack_normals.push_back(tangent_vertices[n]);
		ltrack_normals.push_back(tangent_vertices[n]);
		ltrack_normals.push_back(tangent_vertices[n]);

		
		//vertices for P1 as 2 triangles
		//  V2-----V1								V6-----V5	
		//	| \     |								|  \    |
		//	|   \   |								|    \  |
		//	V3-----V0								V7-----V4

		

		ltrack_vertices.push_back(V7);
		ltrack_vertices.push_back(V4);
		ltrack_vertices.push_back(V6);

		ltrack_normals.push_back(-tangent_vertices[n]);
		ltrack_normals.push_back(-tangent_vertices[n]);
		ltrack_normals.push_back(-tangent_vertices[n]);


		ltrack_vertices.push_back(V6);
		ltrack_vertices.push_back(V4);
		ltrack_vertices.push_back(V5);

		ltrack_normals.push_back(-tangent_vertices[n]);
		ltrack_normals.push_back(-tangent_vertices[n]);
		ltrack_normals.push_back(-tangent_vertices[n]);

		 //repetitive

		//left side vertices for connecting P0 and P1 as 2 triangles :
		/*		  V6-----V5
				 / |      |
				/  |      |
			   /  V7-----V4
			  V2--/--V1 
			  |  /    |	
			  | /     |  
			  V3-----V0

		*/

		ltrack_vertices.push_back(V3);
		ltrack_vertices.push_back(V2);
		ltrack_vertices.push_back(V7);

		ltrack_normals.push_back(-binormal_vertices[n]);
		ltrack_normals.push_back(-binormal_vertices[n]);
		ltrack_normals.push_back(-binormal_vertices[n + 1]);

		ltrack_vertices.push_back(V7);
		ltrack_vertices.push_back(V2); 
		ltrack_vertices.push_back(V6);

		ltrack_normals.push_back(-binormal_vertices[n+1]);
		ltrack_normals.push_back(-binormal_vertices[n]);
		ltrack_normals.push_back(-binormal_vertices[n + 1]);

		//for top side between P0 and P1 2 triangles
		ltrack_vertices.push_back(V1);
		ltrack_vertices.push_back(V2);
		ltrack_vertices.push_back(V5);

		ltrack_normals.push_back(normal_vertices[n]);
		ltrack_normals.push_back(normal_vertices[n]);
		ltrack_normals.push_back(normal_vertices[n + 1]);

		ltrack_vertices.push_back(V5);
		ltrack_vertices.push_back(V2);
		ltrack_vertices.push_back(V6);

		ltrack_normals.push_back(normal_vertices[n + 1]);
		ltrack_normals.push_back(normal_vertices[n]);
		ltrack_normals.push_back(normal_vertices[n + 1]);


		//right side vertices for connecting P0 and P1 as 2 triangles : 
		/*	    V6------V5
			   / |     / |
			  /	 |    /  |
			 /	V7---/--V4
			V2-/--V1/	 /
			| /     |	/
			|/      |  /
			V3-----V0

		*/

		ltrack_vertices.push_back(V0);
		ltrack_vertices.push_back(V4);
		ltrack_vertices.push_back(V1);

		ltrack_normals.push_back(binormal_vertices[n]);
		ltrack_normals.push_back(binormal_vertices[n+1]);
		ltrack_normals.push_back(binormal_vertices[n]);

		ltrack_vertices.push_back(V1);
		ltrack_vertices.push_back(V5); //change
		ltrack_vertices.push_back(V4);

		ltrack_normals.push_back(binormal_vertices[n]);
		ltrack_normals.push_back(binormal_vertices[n + 1]);
		ltrack_normals.push_back(binormal_vertices[n + 1]);


		//for bottom side between P0 and P1 2 triangles
		ltrack_vertices.push_back(V0);
		ltrack_vertices.push_back(V3);
		ltrack_vertices.push_back(V4);

		ltrack_normals.push_back(-normal_vertices[n]);
		ltrack_normals.push_back(-normal_vertices[n]);
		ltrack_normals.push_back(-normal_vertices[n + 1]);

		ltrack_vertices.push_back(V4);
		ltrack_vertices.push_back(V3);
		ltrack_vertices.push_back(V7);

		ltrack_normals.push_back(-normal_vertices[n + 1]);
		ltrack_normals.push_back(-normal_vertices[n]);
		ltrack_normals.push_back(-normal_vertices[n + 1]);


#pragma endregion leftside

#pragma region rightside
		//P0 points
		glm::vec3 rV0 = {P0.x - scale * binormal_vertices[n].x + 0.05f * (-normal_vertices[n].x + binormal_vertices[n].x),
						 P0.y - scale * binormal_vertices[n].y + 0.05f * (-normal_vertices[n].y + binormal_vertices[n].y),
						 P0.z - scale * binormal_vertices[n].z + 0.05f * (-normal_vertices[n].z + binormal_vertices[n].z) };
							   
		glm::vec3 rV1 = {P0.x - scale * binormal_vertices[n].x + 0.05f * (normal_vertices[n].x + binormal_vertices[n].x),
						 P0.y - scale * binormal_vertices[n].y + 0.05f * (normal_vertices[n].y + binormal_vertices[n].y),
						 P0.z - scale * binormal_vertices[n].z + 0.05f * (normal_vertices[n].z + binormal_vertices[n].z) };
							   
		glm::vec3 rV2 = {P0.x - scale * binormal_vertices[n].x + 0.05f * (normal_vertices[n].x - binormal_vertices[n].x),
						 P0.y - scale * binormal_vertices[n].y + 0.05f * (normal_vertices[n].y - binormal_vertices[n].y),
						 P0.z - scale * binormal_vertices[n].z + 0.05f * ( normal_vertices[n].z - binormal_vertices[n].z) };
							   
		glm::vec3 rV3 = {P0.x - scale * binormal_vertices[n].x + 0.05f * (-normal_vertices[n].x + binormal_vertices[n].x),
						 P0.y - scale * binormal_vertices[n].y + 0.05f * (-normal_vertices[n].y + binormal_vertices[n].y),
						 P0.z - scale * binormal_vertices[n].z + 0.05f * (-normal_vertices[n].z + binormal_vertices[n].z) };

		//P1 points
		glm::vec3 rV4 = {P1.x - scale * binormal_vertices[n].x + 0.05f * (-normal_vertices[n].x + binormal_vertices[n].x),
						 P1.y - scale * binormal_vertices[n].y + 0.05f * (-normal_vertices[n].y + binormal_vertices[n].y),
						 P1.z - scale * binormal_vertices[n].z + 0.05f * (-normal_vertices[n].z + binormal_vertices[n].z) };

		glm::vec3 rV5 = {P1.x - scale * binormal_vertices[n].x + 0.05f * (normal_vertices[n].x + binormal_vertices[n].x),
						 P1.y - scale * binormal_vertices[n].y + 0.05f * (normal_vertices[n].y + binormal_vertices[n].y),
						 P1.z - scale * binormal_vertices[n].z + 0.05f * (normal_vertices[n].z + binormal_vertices[n].z) };

		glm::vec3 rV6 = {P1.x - scale * binormal_vertices[n].x + 0.05f * (normal_vertices[n].x - binormal_vertices[n].x),
						 P1.y - scale * binormal_vertices[n].y + 0.05f * (normal_vertices[n].y - binormal_vertices[n].y),
						 P1.z - scale * binormal_vertices[n].z + 0.05f * (normal_vertices[n].z - binormal_vertices[n].z) };

		glm::vec3 rV7 = {P1.x - scale * binormal_vertices[n].x + 0.05f * (-normal_vertices[n].x + binormal_vertices[n].x),
						 P1.y - scale * binormal_vertices[n].y + 0.05f * (-normal_vertices[n].y + binormal_vertices[n].y),
						 P1.z - scale * binormal_vertices[n].z + 0.05f * (-normal_vertices[n].z + binormal_vertices[n].z) };
		
		//P0 triangles 
		rtrack_vertices.push_back(rV3);
		rtrack_vertices.push_back(rV0);
		rtrack_vertices.push_back(rV2);

		rtrack_normals.push_back(tangent_vertices[n]);
		rtrack_normals.push_back(tangent_vertices[n]);
		rtrack_normals.push_back(tangent_vertices[n]);

		rtrack_vertices.push_back(rV2);
		rtrack_vertices.push_back(rV0);
		rtrack_vertices.push_back(rV1);

		rtrack_normals.push_back(tangent_vertices[n]);
		rtrack_normals.push_back(tangent_vertices[n]);
		rtrack_normals.push_back(tangent_vertices[n]);

		//left side of cross section
		rtrack_vertices.push_back(rV3);
		rtrack_vertices.push_back(rV2);
		rtrack_vertices.push_back(rV7);

		rtrack_normals.push_back(-binormal_vertices[n]);
		rtrack_normals.push_back(-binormal_vertices[n]);
		rtrack_normals.push_back(-binormal_vertices[n + 1]);

		rtrack_vertices.push_back(rV7);
		rtrack_vertices.push_back(rV2);
		rtrack_vertices.push_back(rV6);

		rtrack_normals.push_back(-binormal_vertices[n + 1]);
		rtrack_normals.push_back(-binormal_vertices[n]);
		rtrack_normals.push_back(-binormal_vertices[n + 1]);

		//for top surface of cross section
		rtrack_vertices.push_back(rV1);
		rtrack_vertices.push_back(rV2);
		rtrack_vertices.push_back(rV5);

		rtrack_normals.push_back(normal_vertices[n]);
		rtrack_normals.push_back(normal_vertices[n]);
		rtrack_normals.push_back(normal_vertices[n + 1]);

		rtrack_vertices.push_back(rV5);
		rtrack_vertices.push_back(rV2);
		rtrack_vertices.push_back(rV6);

		rtrack_normals.push_back(normal_vertices[n + 1]);
		rtrack_normals.push_back(normal_vertices[n]);
		rtrack_normals.push_back(normal_vertices[n + 1]);

		//right side of cross section
		rtrack_vertices.push_back(rV0);
		rtrack_vertices.push_back(rV1);
		rtrack_vertices.push_back(rV4);

		rtrack_normals.push_back(binormal_vertices[n]);
		rtrack_normals.push_back(binormal_vertices[n]);
		rtrack_normals.push_back(binormal_vertices[n + 1]);

		rtrack_vertices.push_back(rV4);
		rtrack_vertices.push_back(rV1); 
		rtrack_vertices.push_back(rV5);

		rtrack_normals.push_back(binormal_vertices[n + 1]);
		rtrack_normals.push_back(binormal_vertices[n]);
		rtrack_normals.push_back(binormal_vertices[n + 1]);

		//bottom surface of cross section
		rtrack_vertices.push_back(rV0);
		rtrack_vertices.push_back(rV3);
		rtrack_vertices.push_back(rV4);

		rtrack_normals.push_back(-normal_vertices[n]);
		rtrack_normals.push_back(-normal_vertices[n]);
		rtrack_normals.push_back(-normal_vertices[n + 1]);

		rtrack_vertices.push_back(rV4);
		rtrack_vertices.push_back(rV3);
		rtrack_vertices.push_back(rV7);

		rtrack_normals.push_back(-normal_vertices[n + 1]);
		rtrack_normals.push_back(-normal_vertices[n]);
		rtrack_normals.push_back(-normal_vertices[n + 1]);
				
#pragma endregion rightside

	}

#pragma region bars in between

	for (int i = 0; i < spline_point.size() - 50; i = i + 20) {

		glm::vec3 cur_left_point =  {spline_point[i].x + scale * binormal_vertices[i].x,
									 spline_point[i].y + scale * binormal_vertices[i].y, 
									 spline_point[i].z + scale * binormal_vertices[i].z };

		glm::vec3 cur_right_point = {spline_point[i].x - scale * binormal_vertices[i].x, 
									 spline_point[i].y - scale * binormal_vertices[i].y, 
									 spline_point[i].z - scale * binormal_vertices[i].z };
		int counter = 0;
		//texture coordinates
		glm::vec2 CR = { 20.0f, 0.0f };
		glm::vec2 NR = { 20.0f, 20.0f };
		glm::vec2 NL = { 0.0f, 20.0f };
		glm::vec2 CL = { 0.0f, 0.0f };

		for (int j = i + 1; counter < 5; j++)
		{
			glm::vec3 next_left_point = {   spline_point[j].x + scale * binormal_vertices[j].x,
											spline_point[j].y + scale * binormal_vertices[j].y,
											spline_point[j].z + scale * binormal_vertices[j].z };

			glm::vec3 next_right_point = {  spline_point[j].x - scale * binormal_vertices[j].x,
											spline_point[j].y - scale * binormal_vertices[j].y,
											spline_point[j].z - scale * binormal_vertices[j].z };

			counter++;

			//triangle 1
			bars_vertices.push_back(cur_left_point);
			bars_vertices.push_back(cur_right_point);
			bars_vertices.push_back(next_left_point);

			bars_texCoords.push_back(CL[0]);
			bars_texCoords.push_back(CL[1]);
			bars_texCoords.push_back(CR[0]);
			bars_texCoords.push_back(CR[1]);
			bars_texCoords.push_back(NL[0]);
			bars_texCoords.push_back(NL[1]);

			//triangle 2
			bars_vertices.push_back(next_left_point);
			bars_vertices.push_back(next_right_point);
			bars_vertices.push_back(cur_right_point);

			bars_texCoords.push_back(NL[0]);
			bars_texCoords.push_back(NL[1]);
			bars_texCoords.push_back(NR[0]);
			bars_texCoords.push_back(NR[1]);
			bars_texCoords.push_back(CR[0]);
			bars_texCoords.push_back(CR[1]);

			cur_left_point = next_left_point;
			cur_right_point = next_right_point;
		}

	}

#pragma endregion bars in between

	cout << "leaving initTrack" << endl;
}

void initGround()
{
	//ground in XY plane
	glm::vec3 leftUp	= { -512, 512,-1 };
	glm::vec3 leftDown	= { -512,-512,-1 };
	glm::vec3 rightUp	= {  512, 512,-1 };
	glm::vec3 rightDown	= {  512,-512,-1 };

	glm::vec2 texLD = { 0.0, 0.0 };
	glm::vec2 texLU = { 0.0, 200.0 };
	glm::vec2 texRD = { 200.0, 0.0 };
	glm::vec2 texRU = { 200.0, 200.0 };

	//draw triangles 
	ground_vertices.push_back(leftDown);
	ground_vertices.push_back(rightDown);
	ground_vertices.push_back(leftUp);

	ground_texCoords.push_back(texLD.x);
	ground_texCoords.push_back(texLD.y);
	ground_texCoords.push_back(texRD.x);
	ground_texCoords.push_back(texRD.y);
	ground_texCoords.push_back(texLU.x);
	ground_texCoords.push_back(texLU.y);


	ground_vertices.push_back(leftUp);
	ground_vertices.push_back(rightDown);
	ground_vertices.push_back(rightUp);

	ground_texCoords.push_back(texLU.x);
	ground_texCoords.push_back(texLU.y);
	ground_texCoords.push_back(texRD.x);
	ground_texCoords.push_back(texRD.y);
	ground_texCoords.push_back(texRU.x);
	ground_texCoords.push_back(texRU.y);


	glGenTextures(1, &h_groundTex);
	initTexture("texture/ground.jpg", h_groundTex);

	glGenVertexArrays(1, &groundVAO);
	glBindVertexArray(groundVAO);

	glGenBuffers(1, &groundVBO);
	glBindBuffer(GL_ARRAY_BUFFER, groundVBO);

	glBufferData(GL_ARRAY_BUFFER, (ground_vertices.size() * sizeof(glm::vec3) + ground_texCoords.size() * sizeof(GLfloat)), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, ground_vertices.size() * sizeof(glm::vec3), &ground_vertices[0]);
	glBufferSubData(GL_ARRAY_BUFFER, ground_vertices.size() * sizeof(glm::vec3), ground_texCoords.size() * sizeof(GLfloat), &ground_texCoords[0]);
   
}

void initSkybox()
{
	//compute cube
	glm::vec3 frontBL  = {	-128.0,	-128.0, -128.0 };
	glm::vec3 frontBR  = {	128.0,	-128.0, -128.0 };
	glm::vec3 frontTL  = {	-128.0,	 128.0, -128.0 };
	glm::vec3 frontTR  = {	 128.0,	 128.0, -128.0 };

	glm::vec3 backBL	= {	-128.0,	-128.0, 128.0 };
	glm::vec3 backBR	= {	128.0,	-128.0, 128.0 };
	glm::vec3 backTL	= {	-128.0,	 128.0, 128.0 };
	glm::vec3 backTR	= {	128.0,	 128.0, 128.0 }; //Z

	glm::vec3 leftBL    = { 128.0,	-128.0,	 128.0 };
	glm::vec3 leftBR    = { 128.0,	-128.0,	-128.0 };
	glm::vec3 leftTL    = { 128.0,	128.0,	 128.0 };
	glm::vec3 leftTR    = { 128.0,	128.0,	-128.0 };

	glm::vec3 rightBL	= { -128.0,	-128.0,	 128.0 };
	glm::vec3 rightBR	= { -128.0,	-128.0,	-128.0 };
	glm::vec3 rightTL	= { -128.0,	128.0,	 128.0 };
	glm::vec3 rightTR	= { -128.0,	128.0,	-128.0 }; //x

	glm::vec3 topBL		= { -128.0, -128.0,  128.0 };
	glm::vec3 topBR		= { 128.0,  -128.0,  128.0 };
	glm::vec3 topTL		= { -128.0, -128.0, -128.0 };
	glm::vec3 topTR		= { 128.0,  -128.0, -128.0 };

	glm::vec3 bottomBL	= {	-128.0, 128.0,  128.0 };
	glm::vec3 bottomBR	= {	 128.0,	128.0,  128.0 };
	glm::vec3 bottomTL	= {	-128.0, 128.0, -128.0 };
	glm::vec3 bottomTR	= {	 128.0,	128.0, -128.0 };


	//tex coordinates
	glm::vec2 texBL = { 0.0, 0.0 };
	glm::vec2 texBR = { 0.0, 1.0 };
	glm::vec2 texTL = { 1.0, 1.0 };
	glm::vec2 texTR = { 1.0, 0.0 };	

	//push vertices
#pragma region frontface
	skybox_vertices.push_back(frontBL);
	skybox_vertices.push_back(frontBR);
	skybox_vertices.push_back(frontTL);

	skybox_texCoords.push_back(texBL[0]);
	skybox_texCoords.push_back(texBL[1]);
	skybox_texCoords.push_back(texBR[0]);
	skybox_texCoords.push_back(texBR[1]);
	skybox_texCoords.push_back(texTL[0]);
	skybox_texCoords.push_back(texTL[1]);

	skybox_vertices.push_back(frontTL);
	skybox_vertices.push_back(frontBR);
	skybox_vertices.push_back(frontTR);

	skybox_texCoords.push_back(texTL[0]);
	skybox_texCoords.push_back(texTL[1]);
	skybox_texCoords.push_back(texBR[0]);
	skybox_texCoords.push_back(texBR[1]);
	skybox_texCoords.push_back(texTR[0]);
	skybox_texCoords.push_back(texTR[1]);
#pragma endregion frontface

#pragma region backface
	skybox_vertices.push_back(backBL);
	skybox_vertices.push_back(backBR);
	skybox_vertices.push_back(backTL);

	skybox_texCoords.push_back(texTL[0]);
	skybox_texCoords.push_back(texTL[1]);
	skybox_texCoords.push_back(texTR[0]);
	skybox_texCoords.push_back(texTR[1]);
	skybox_texCoords.push_back(texBL[0]);
	skybox_texCoords.push_back(texBL[1]);

	skybox_vertices.push_back(backTL);
	skybox_vertices.push_back(backBR);
	skybox_vertices.push_back(backTR);

	skybox_texCoords.push_back(texBL[0]);
	skybox_texCoords.push_back(texBL[1]);
	skybox_texCoords.push_back(texTR[0]);
	skybox_texCoords.push_back(texTR[1]);
	skybox_texCoords.push_back(texBR[0]);
	skybox_texCoords.push_back(texBR[1]);
#pragma endregion backface

#pragma region leftface
	skybox_vertices.push_back(leftBL);
	skybox_vertices.push_back(leftBR);
	skybox_vertices.push_back(leftTL);

	skybox_texCoords.push_back(texBL[0]);
	skybox_texCoords.push_back(texBL[1]);
	skybox_texCoords.push_back(texBR[0]);
	skybox_texCoords.push_back(texBR[1]);
	skybox_texCoords.push_back(texTL[0]);
	skybox_texCoords.push_back(texTL[1]);

	skybox_vertices.push_back(leftTL);
	skybox_vertices.push_back(leftBR);
	skybox_vertices.push_back(leftTR);

	skybox_texCoords.push_back(texTL[0]);
	skybox_texCoords.push_back(texTL[1]);
	skybox_texCoords.push_back(texBR[0]);
	skybox_texCoords.push_back(texBR[1]);
	skybox_texCoords.push_back(texTR[0]);
	skybox_texCoords.push_back(texTR[1]);
#pragma endregion leftface

#pragma region rightface
	skybox_vertices.push_back(rightBL);
	skybox_vertices.push_back(rightBR);
	skybox_vertices.push_back(rightTL);

	skybox_texCoords.push_back(texTL[0]);
	skybox_texCoords.push_back(texTL[1]);
	skybox_texCoords.push_back(texTR[0]);
	skybox_texCoords.push_back(texTR[1]);
	skybox_texCoords.push_back(texBL[0]);
	skybox_texCoords.push_back(texBL[1]);

	skybox_vertices.push_back(rightTL);
	skybox_vertices.push_back(rightBR);
	skybox_vertices.push_back(rightTR);

	skybox_texCoords.push_back(texBL[0]);
	skybox_texCoords.push_back(texBL[1]);
	skybox_texCoords.push_back(texTR[0]);
	skybox_texCoords.push_back(texTR[1]);
	skybox_texCoords.push_back(texBR[0]);
	skybox_texCoords.push_back(texBR[1]);
#pragma endregion rightface

#pragma region topface
	skybox_vertices.push_back(topBL);
	skybox_vertices.push_back(topBR);
	skybox_vertices.push_back(topTL);

	skybox_texCoords.push_back(texTL[0]);
	skybox_texCoords.push_back(texTL[1]);
	skybox_texCoords.push_back(texTR[0]);
	skybox_texCoords.push_back(texTR[1]);
	skybox_texCoords.push_back(texBL[0]);
	skybox_texCoords.push_back(texBL[1]);

	skybox_vertices.push_back(topTL);
	skybox_vertices.push_back(topBR);
	skybox_vertices.push_back(topTR);

	skybox_texCoords.push_back(texBL[0]);
	skybox_texCoords.push_back(texBL[1]);
	skybox_texCoords.push_back(texTR[0]);
	skybox_texCoords.push_back(texTR[1]);
	skybox_texCoords.push_back(texBR[0]);
	skybox_texCoords.push_back(texBR[1]);
#pragma endregion topface

#pragma region bottomface
	skybox_vertices.push_back(bottomBL);
	skybox_vertices.push_back(bottomBR);
	skybox_vertices.push_back(bottomTL);

	skybox_texCoords.push_back(texBL[0]);
	skybox_texCoords.push_back(texBL[1]);
	skybox_texCoords.push_back(texBR[0]);
	skybox_texCoords.push_back(texBR[1]);
	skybox_texCoords.push_back(texTL[0]);
	skybox_texCoords.push_back(texTL[1]);

	skybox_vertices.push_back(bottomTL);
	skybox_vertices.push_back(bottomBR);
	skybox_vertices.push_back(bottomTR);

	skybox_texCoords.push_back(texTL[0]);
	skybox_texCoords.push_back(texTL[1]);
	skybox_texCoords.push_back(texBR[0]);
	skybox_texCoords.push_back(texBR[1]);
	skybox_texCoords.push_back(texTR[0]);
	skybox_texCoords.push_back(texTR[1]);
#pragma endregion bottomface

	glGenTextures(1, &h_skyTex);
	initTexture("texture/nightsky.jpg", h_skyTex);

	glGenVertexArrays(1, &skyboxVAO);
	glBindVertexArray(skyboxVAO);

	glGenBuffers(1, &skyboxVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);

	glBufferData(GL_ARRAY_BUFFER, (skybox_vertices.size() * sizeof(glm::vec3) + skybox_texCoords.size() * sizeof(GLfloat)), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, skybox_vertices.size() * sizeof(glm::vec3), &skybox_vertices[0]);
	glBufferSubData(GL_ARRAY_BUFFER, skybox_vertices.size() * sizeof(glm::vec3), skybox_texCoords.size() * sizeof(GLfloat), &skybox_texCoords[0]);

}

void initTrackVBO()
{
	//glGenTextures(1, &h_railTex);
	//initTexture("texture/Metal.jpg", h_railTex);

	//left track
	glGenVertexArrays(1, &ltrackVAO);
	glBindVertexArray(ltrackVAO);

	glGenBuffers(1, &ltrackVBO);
	glBindBuffer(GL_ARRAY_BUFFER, ltrackVBO);
	
	//glBufferData(GL_ARRAY_BUFFER, (spline_point.size() * sizeof(glm::vec3) + spline_colors.size() * sizeof(glm::vec4)), NULL, GL_STATIC_DRAW);
	//glBufferSubData(GL_ARRAY_BUFFER, 0, spline_point.size() * sizeof(glm::vec3), &spline_point[0]);
	//glBufferSubData(GL_ARRAY_BUFFER, spline_point.size() * sizeof(glm::vec3), spline_colors.size() * sizeof(glm::vec4), &spline_colors[0]);

	//glBufferData(GL_ARRAY_BUFFER, ( ltrack_vertices.size() * sizeof(glm::vec3) + ltrack_texCoords.size() * sizeof(GLfloat)), NULL, GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, (ltrack_vertices.size() * sizeof(glm::vec3) + ltrack_normals.size() * sizeof(glm::vec3)), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, ltrack_vertices.size() * sizeof(glm::vec3), &ltrack_vertices[0]);
	glBufferSubData(GL_ARRAY_BUFFER, ltrack_vertices.size() * sizeof(glm::vec3), ltrack_normals.size() * sizeof(glm::vec3), &ltrack_normals[0]);

	//right track
	glGenVertexArrays(1, &rtrackVAO);
	glBindVertexArray(rtrackVAO);

	glGenBuffers(1, &rtrackVBO);
	glBindBuffer(GL_ARRAY_BUFFER, rtrackVBO);

	glBufferData(GL_ARRAY_BUFFER, (rtrack_vertices.size() * sizeof(glm::vec3) + rtrack_normals.size() * sizeof(glm::vec3)), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, rtrack_vertices.size() * sizeof(glm::vec3), &rtrack_vertices[0]);
	glBufferSubData(GL_ARRAY_BUFFER, rtrack_vertices.size() * sizeof(glm::vec3), rtrack_normals.size() * sizeof(glm::vec3), &rtrack_normals[0]);

	//bars
	glGenTextures(1, &h_barTex);
	initTexture("texture/metal.jpg", h_barTex);

	glGenVertexArrays(1, &barsVAO);
	glBindVertexArray(barsVAO);

	glGenBuffers(1, &barsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, barsVBO);

	glBufferData(GL_ARRAY_BUFFER, (bars_vertices.size() * sizeof(glm::vec3) + bars_texCoords.size() * sizeof(GLfloat)), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, bars_vertices.size() * sizeof(glm::vec3), &bars_vertices[0]);
	glBufferSubData(GL_ARRAY_BUFFER, bars_vertices.size() * sizeof(glm::vec3), bars_texCoords.size() * sizeof(GLfloat), &bars_texCoords[0]);
}

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
	unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
	glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

	ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

	if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
		cout << "File " << filename << " saved successfully." << endl;
	else cout << "Failed to save file " << filename << '.' << endl;

	delete[] screenshotData;
}

void displayFunc()
{
	// render some stuff...
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	//model view
	openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
	openGLMatrix.LoadIdentity();

	glm::vec3 eye = { spline_point[counter].x + 0.2 * normal_vertices[counter].x, 
					  spline_point[counter].y + 0.2 * normal_vertices[counter].y, 
					  spline_point[counter].z + 0.2 * normal_vertices[counter].z };
		//spline_point[counter] + normal_vertices[counter]; //vector from normal to point
		//{ spline_point[counter].x + 0.2, spline_point[counter].y + 0.2, spline_point[counter].z + 0.2 };
	glm::vec3 focus =	spline_point[counter] + tangent_vertices[counter]; //along the point and tangent 
	glm::vec3 up =		{ normal_vertices[counter].x * 0.25 , normal_vertices[counter].y * 0.25, normal_vertices[counter].z * 0.25 }; //look down to 

	openGLMatrix.LookAt(eye.x, eye.y, eye.z, focus.x, focus.y, focus.z, up.x, up.y, up.z);

	//enable transformations on the rednering
	openGLMatrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
	openGLMatrix.Rotate(landRotate[0], 1.0f, 0.0f, 0.0f);
	openGLMatrix.Rotate(landRotate[1], 0.0f, 1.0f, 0.0f);
	openGLMatrix.Rotate(landRotate[2], 0.0f, 0.0f, 1.0f);
	openGLMatrix.Scale(landScale[0], landScale[1], landScale[2]);

	//PHONG SHADING
	phongPipelineProg.Bind();

	h_modelMatrix = glGetUniformLocation(phong_program_ID, "modelViewMatrix");
	h_projection = glGetUniformLocation(phong_program_ID, "projectionMatrix");
	h_lightDirection = glGetUniformLocation(phong_program_ID, "viewLightDirection");
	h_normal = glGetUniformLocation(phong_program_ID, "normalMatrix");

	openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
	openGLMatrix.GetMatrix(m);
	glUniformMatrix4fv(h_modelMatrix, 1, GL_FALSE, m);

	//projection
	openGLMatrix.SetMatrixMode(OpenGLMatrix::Projection);
	openGLMatrix.GetMatrix(p);
	glUniformMatrix4fv(h_projection, 1, GL_FALSE, p);

	//normal
	openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
	openGLMatrix.GetNormalMatrix(n);
	glUniformMatrix4fv(h_normal, 1, GL_FALSE, n);

	openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
	openGLMatrix.GetMatrix(view);

	//lightDirection
	glm::vec4 lightDirection = { 1.0f, 0.0f, 0.0f, 0.0f }; // the “Sun” at noon
	glm::mat4 viewMat = glm::make_mat4(view);
	glm::vec3 viewLightDirection(viewMat * lightDirection); // light direction in the view space
	glUniform3fv(h_lightDirection, 1, glm::value_ptr(viewLightDirection));

	GLuint loc;

#pragma region render left track
	//glBindTexture(GL_TEXTURE_2D, h_railTex);

	glBindVertexArray(ltrackVAO);
	glBindBuffer(GL_ARRAY_BUFFER, ltrackVBO);
	loc = glGetAttribLocation(phong_program_ID, "position");
	glEnableVertexAttribArray(loc);
	const void * offset = (const void*)0;
	GLsizei stride = 0;
	GLboolean normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	loc = glGetAttribLocation(phong_program_ID, "normal");
	glEnableVertexAttribArray(loc);
	offset = (const void*)(ltrack_vertices.size() * sizeof(glm::vec3));
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	int count = ltrack_vertices.size();
	glDrawArrays(GL_TRIANGLES, 0, count);

	glDisableVertexAttribArray(loc);

#pragma endregion render left track

#pragma region render right track
	//glBindTexture(GL_TEXTURE_2D, h_railTex);

	glBindVertexArray(rtrackVAO);
	glBindBuffer(GL_ARRAY_BUFFER, rtrackVBO);
	loc = glGetAttribLocation(phong_program_ID, "position");
	glEnableVertexAttribArray(loc);
	offset = (const void*)0;
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	loc = glGetAttribLocation(phong_program_ID, "normal");
	glEnableVertexAttribArray(loc);
	offset = (const void*)(rtrack_vertices.size() * sizeof(glm::vec3));
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	count = rtrack_vertices.size();
	glDrawArrays(GL_TRIANGLES, 0, count);

	glDisableVertexAttribArray(loc);

#pragma endregion render right track


	//BASIC SHADING
	objPipelineProg.Bind();

	h_texModelViewMatrix = glGetUniformLocation(program_ID, "modelViewMatrix");
	h_texProjectionMatrix = glGetUniformLocation(program_ID, "projectionMatrix");

	
	// get model view matrix
	openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
	openGLMatrix.GetMatrix(m);
	glUniformMatrix4fv(h_texModelViewMatrix, 1, GL_FALSE, m);

	// get projection matrix
	openGLMatrix.SetMatrixMode(OpenGLMatrix::Projection);
	openGLMatrix.GetMatrix(p);
	glUniformMatrix4fv(h_texProjectionMatrix, 1, GL_FALSE, p);

	GLuint loc2;

#pragma region render skybox

	glBindTexture(GL_TEXTURE_2D, h_skyTex);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	loc2 = glGetAttribLocation(program_ID, "position");
	glEnableVertexAttribArray(loc2);
	const void * offset2 = (const void*)0;
	GLsizei stride2 = 0;
	GLboolean normalized2 = GL_FALSE;
	glVertexAttribPointer(loc2, 3, GL_FLOAT, normalized2, stride2, offset2);

	loc2 = glGetAttribLocation(program_ID, "texCoord");
	glEnableVertexAttribArray(loc2);
	offset2 = (const void*)(skybox_vertices.size() * sizeof(glm::vec3));
	glVertexAttribPointer(loc2, 2, GL_FLOAT, normalized2, stride2, offset2);

	int count2 = skybox_vertices.size();
	glDrawArrays(GL_TRIANGLES, 0, count2);
	
	glDisableVertexAttribArray(loc2);

#pragma endregion render skybox

#pragma region render ground

	glBindTexture(GL_TEXTURE_2D, h_groundTex);
	glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
	loc2 = glGetAttribLocation(program_ID, "position");
	glEnableVertexAttribArray(loc2);
	offset2 = (const void*)0;
	stride2 = 0;
	normalized2 = GL_FALSE;
	glVertexAttribPointer(loc2, 3, GL_FLOAT, normalized2, stride2, offset2);

	loc2 = glGetAttribLocation(program_ID, "texCoord");
	glEnableVertexAttribArray(loc2);
	offset2 = (const void*)(ground_vertices.size() * sizeof(glm::vec3));
	glVertexAttribPointer(loc2, 2, GL_FLOAT, normalized2, stride2, offset2);

	count2 = bars_vertices.size();
	glDrawArrays(GL_TRIANGLES, 0, count2);

	glDisableVertexAttribArray(loc2);
#pragma endregion render ground

#pragma region render bars in between
	glBindTexture(GL_TEXTURE_2D, h_barTex);
	glBindBuffer(GL_ARRAY_BUFFER, barsVBO);
	loc2 = glGetAttribLocation(program_ID, "position");
	glEnableVertexAttribArray(loc2);
	offset2 = (const void*)0;
	stride2 = 0;
	normalized2 = GL_FALSE;
	glVertexAttribPointer(loc2, 3, GL_FLOAT, normalized2, stride2, offset2);

	loc2 = glGetAttribLocation(program_ID, "texCoord");
	glEnableVertexAttribArray(loc2);
	offset2 = (const void*)(bars_vertices.size() * sizeof(glm::vec3));
	glVertexAttribPointer(loc2, 2, GL_FLOAT, normalized2, stride2, offset2);

	count2 = bars_vertices.size();
	glDrawArrays(GL_TRIANGLES, 0, count2);

	glDisableVertexAttribArray(loc2);

#pragma endregion render bars in between

	glutSwapBuffers();

}

void idleFunc()
{
	if (counter < spline_point.size() - 4)
	{
		counter++;
		/*if ( screenshots <= 1000) {
			char anim_num[5];
			sprintf(anim_num, "%03d", counter);
			saveScreenshot(("animation/" + std::string(anim_num) + ".jpg").c_str());
			screenshots++;}
		*/
	}

	glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
	glViewport(0, 0, w, h);

	// setup perspective matrix...
	openGLMatrix.SetMatrixMode(OpenGLMatrix::Projection);
	openGLMatrix.LoadIdentity();
	openGLMatrix.Perspective(45.0, 1280.0 / 720.0f, 0.01, 1000.0);
	openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
}

void mouseMotionDragFunc(int x, int y)
{
	// mouse has moved and one of the mouse buttons is pressed (dragging)

	// the change in mouse position since the last invocation of this function
	int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

	switch (controlState)
	{
		// translate the landscape
	case TRANSLATE:
		if (leftMouseButton)
		{
			// control x,y translation via the left mouse button
			landTranslate[0] += mousePosDelta[0] * 0.1f;
			landTranslate[1] -= mousePosDelta[1] * 0.1f;
		}
		if (middleMouseButton)
		{
			// control z translation via the middle mouse button
			landTranslate[2] += mousePosDelta[1] * 0.1f;
		}
		break;

		// rotate the landscape
	case ROTATE:
		if (leftMouseButton)
		{
			// control x,y rotation via the left mouse button
			landRotate[0] += mousePosDelta[1];
			landRotate[1] += mousePosDelta[0];
		}
		if (middleMouseButton)
		{
			// control z rotation via the middle mouse button
			landRotate[2] += mousePosDelta[1];
		}
		break;

		// scale the landscape
	case SCALE:
		if (leftMouseButton)
		{
			// control x,y scaling via the left mouse button
			landScale[0] *= 1.0f + mousePosDelta[0] * 0.1f;
			landScale[1] *= 1.0f - mousePosDelta[1] * 0.1f;
		}
		if (middleMouseButton)
		{
			// control z scaling via the middle mouse button
			landScale[2] *= 1.0f - mousePosDelta[1] * 0.1f;
		}
		break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
	// mouse has moved
	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
	// a mouse button has has been pressed or depressed

	// keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		leftMouseButton = (state == GLUT_DOWN);
		break;

	case GLUT_MIDDLE_BUTTON:
		middleMouseButton = (state == GLUT_DOWN);
		break;

	case GLUT_RIGHT_BUTTON:
		rightMouseButton = (state == GLUT_DOWN);
		break;
	}

	// keep track of whether CTRL and SHIFT keys are pressed
	switch (glutGetModifiers())
	{
	case GLUT_ACTIVE_CTRL:
		controlState = TRANSLATE;
		break;

	case GLUT_ACTIVE_SHIFT:
		controlState = SCALE;
		break;

		// if CTRL and SHIFT are not pressed, we are in rotate mode
	default:
		controlState = ROTATE;
		break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27: // ESC key
		exit(0); // exit the program
		break;

	case ' ':
		cout << "You pressed the spacebar." << endl;
		break;

	case 'x':
		// take a screenshot
		saveScreenshot("screenshot.jpg");
		break;
	}
}

void initScene(int argc, char *argv[])
{
	// load the image from a jpeg disk file to main memory

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);

	objPipelineProg.Init("../openGLHelper-starterCode");
	objPipelineProg.Bind();

	program_ID = objPipelineProg.GetProgramHandle();
	//glUseProgram(program_ID);

	cout << "Basic shaders have been initialised and compiled." << endl;

	phongPipelineProg.Init("../openGLHelper-starterCode");
	phong_program_ID = phongPipelineProg.GetProgramHandle();

	cout << "Phong shaders have been initialised and compiled." << endl;

	initSplineVertices();
	initSkybox();
	initTrackVertices();
	initTrackVBO();
	initGround();
	
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("usage: %s <trackfile>\n", argv[0]);
		exit(0);
	}

	// load the splines from the provided filename
	loadSplines(argv[1]);

	printf("Loaded %d spline(s).\n", numSplines);
	for (int i = 0; i < numSplines; i++)
		printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);

	cout << "Initializing GLUT..." << endl;
	glutInit(&argc, argv);

	cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(0, 0);
	glutCreateWindow(windowTitle);

	cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
	cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
	cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	// tells glut to use a particular display function to redraw 
	glutDisplayFunc(displayFunc);
	// perform animation inside idleFunc
	glutIdleFunc(idleFunc);
	// callback for mouse drags
	glutMotionFunc(mouseMotionDragFunc);
	// callback for idle mouse movement
	glutPassiveMotionFunc(mouseMotionFunc);
	// callback for mouse button changes
	glutMouseFunc(mouseButtonFunc);
	// callback for resizing the window
	glutReshapeFunc(reshapeFunc);
	// callback for pressing the keys on the keyboard
	glutKeyboardFunc(keyboardFunc);

	// init glew
#ifdef __APPLE__
  // nothing is needed on Apple
#else
  // Windows, Linux
	GLint result = glewInit();
	if (result != GLEW_OK)
	{
		cout << "error: " << glewGetErrorString(result) << endl;
		exit(EXIT_FAILURE);
	}
#endif

	// do initialization
	initScene(argc, argv);

	// sink forever into the glut loop
	glutMainLoop();
}


