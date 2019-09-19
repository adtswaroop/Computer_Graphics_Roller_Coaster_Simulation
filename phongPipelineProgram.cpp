#include "phongPipelineProgram.h"
#include "openGLHeader.h"

#include <iostream>
#include <cstring>

using namespace std;

int PhongPipelineProgram::Init(const char * shaderBasePath)
{
	if (BuildShadersFromFiles(shaderBasePath, "phong.vertexShader.glsl", "phong.fragmentShader.glsl") != 0)
	{
		cout << "Failed to build the pipeline program." << endl;
		return 1;
	}

	cout << "Successfully built the pipeline program." << endl;
	return 0;
}

void PhongPipelineProgram::SetModelViewMatrix(const float * m)
{
	// pass "m" to the pipeline program, as the modelview matrix
	// students need to implement this
	GLboolean isRowMajor = GL_FALSE;
	glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);
}

void PhongPipelineProgram::SetProjectionMatrix(const float * m)
{
	// pass "m" to the pipeline program, as the projection matrix
	// students need to implement this
	GLboolean isRowMajor = GL_FALSE;
	glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, m);
}

int PhongPipelineProgram::SetShaderVariableHandles()
{
	// set h_modelViewMatrix and h_projectionMatrix
	// students need to implement this
	SET_SHADER_VARIABLE_HANDLE(modelViewMatrix);
	SET_SHADER_VARIABLE_HANDLE(projectionMatrix);
	return 0;
}

