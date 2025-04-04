#pragma once
#include <GL\glew.h>

class Texture
{
public:
	Texture();
	Texture(const char* fileLoc);

	bool LoadTextureA();
	bool LoadTexture();

	void UseTexture();
	void ClearTexture();

	~Texture();

private:
	GLuint textureID;
	int width, height, bitDepth;

	const char* fileLocation;
};

