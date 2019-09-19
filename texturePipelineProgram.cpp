#include "texturePipelineProgram.h"
#include "openGLHeader.h"

#include <iostream>
#include <cstring>

using namespace std;

int TexturePipelineProgram::Init(const char * shaderBasePath)
{
	if (BuildShadersFromFiles(shaderBasePath, "Texture.vertexShader.glsl", "basicTexture.fragmentShader.glsl") != 0)
	{
		cout << "Failed to build the pipeline program." << endl;
		return 1;
	}

	cout << "Successfully built the pipeline program." << endl;
	return 0;
}

void TexturePipelineProgram::SetModelViewMatrix(const float * m)
{
	// pass "m" to the pipeline program, as the modelview matrix
	// students need to implement this
	GLboolean isRowMajor = GL_FALSE;
	glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);
}

void TexturePipelineProgram::SetProjectionMatrix(const float * m)
{
	// pass "m" to the pipeline program, as the projection matrix
	// students need to implement this
	GLboolean isRowMajor = GL_FALSE;
	glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, m);
}

int TexturePipelineProgram::SetShaderVariableHandles()
{
	// set h_modelViewMatrix and h_projectionMatrix
	// students need to implement this
	SET_SHADER_VARIABLE_HANDLE(modelViewMatrix);
	SET_SHADER_VARIABLE_HANDLE(projectionMatrix);
	return 0;
}

