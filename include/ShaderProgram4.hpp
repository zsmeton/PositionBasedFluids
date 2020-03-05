/** @file ShaderProgram3.hpp
  * @brief Class to work with OpenGL 3.0+ Shaders
	* @author Dr. Jeffrey Paone
	* @date Last Edit: 03 Nov 2017
	* @version 1.4
	*
	* @copyright MIT License Copyright (c) 2017 Dr. Jeffrey Paone
	*
	*	These functions, classes, and constants help minimize common
	*	code that needs to be written.
  */

#ifndef __CSCI444_SHADEREPROGRAM_4_H__
#define __CSCI444_SHADEREPROGRAM_4_H__

#include "ShaderUtils4.hpp"

#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////

/** @namespace CSCI444
  * @brief CSCI444 Helper Functions for OpenGL
	*/
namespace CSCI444 {

	/** @class ShaderProgram
		* @brief Handles registration and compilation of Shaders
		*/
	class ShaderProgram {
	public:
		/** @brief Enables debug messages from Shader Program functions
		  *
			* Enables debug messages from Shader Program functions.  Debug messages are on by default.
		  */
		static void enableDebugMessages();
		/** @brief Disables debug messages from Shader Program functions
		  *
			* Disables debug messages from Shader Program functions.  Debug messages are on by default.
		  */
		static void disableDebugMessages();

        /** @brief Enables shaders being seperable
          *
            * Seperability is set to false by default.
          */
        static void enableSeparablePrograms();
        /** @brief Disables shaders being seperable
          *
            * Seperability is set to false by default.
          */
        static void disableSeparablePrograms();

		/** @brief Creates a Shader Program using a Vertex Shader and Fragment Shader
		  *
			* @param const char* vertexShaderFilename - name of the file corresponding to the vertex shader
			* @param const char* fragmentShaderFilename - name of the file corresponding to the fragment shader
		  */
		ShaderProgram( const char *vertexShaderFilename,
					   			 const char *fragmentShaderFilename );

	  /** @brief Creates a Shader Program using a Vertex Shader, Tesselation Shader, Geometry Shader, and Fragment Shader
			*
			* @param const char* vertexShaderFilename - name of the file corresponding to the vertex shader
			* @param const char* tesselationControlShaderFilename - name of the file corresponding to the tesselation control shader
			* @param const char* tesselationEvaluationShaderFilename - name of the file corresponding to the tesselation evaluation shader
			* @param const char* geometryShaderFilename - name of the file corresponding to the geometry shader
			* @param const char* fragmentShaderFilename - name of the file corresponding to the fragment shader
			*/
		ShaderProgram( const char *vertexShaderFilename,
					   			 const char *tesselationControlShaderFilename,
					   	 		 const char *tesselationEvaluationShaderFilename,
					   	 		 const char *geometryShaderFilename,
					   	 		 const char *fragmentShaderFilename );

		/** @brief Creates a Shader Program using a Vertex Shader, Tesselation Shader, and Fragment Shader
 			*
 			* @param const char* vertexShaderFilename - name of the file corresponding to the vertex shader
 			* @param const char* tesselationControlShaderFilename - name of the file corresponding to the tesselation control shader
 			* @param const char* tesselationEvaluationShaderFilename - name of the file corresponding to the tesselation evaluation shader
 			* @param const char* fragmentShaderFilename - name of the file corresponding to the fragment shader
 			*/
 		ShaderProgram( const char *vertexShaderFilename,
 					   			 const char *tesselationControlShaderFilename,
 					   	 		 const char *tesselationEvaluationShaderFilename,
 					   	 		 const char *fragmentShaderFilename );

		/** @brief Creates a Shader Program using a Vertex Shader, Geometry Shader, and Fragment Shader
		  *
			* @param const char* vertexShaderFilename - name of the file corresponding to the vertex shader
			* @param const char* geometryShaderFilename - name of the file corresponding to the geometry shader
			* @param const char* fragmentShaderFilename - name of the file corresponding to the fragment shader
			*/
		ShaderProgram( const char *vertexShaderFilename,
					   			 const char *geometryShaderFilename,
					   	 		 const char *fragmentShaderFilename );

        /** @brief Creates a Shader Program using a Vertex Shader, Tesselation Shader, Geometry Shader, and Fragment Shader
            *
            * @param const char* shaderFilenames - names of the file corresponding to each shader
            * @param GLbitfield stages - specifies a set of program stages to bind to the program pipeline object
            */
        ShaderProgram( const char *shaderFilenames[], GLbitfield stages);

		/** @brief Clean up memory associated with the Shader Program
 			*/
		~ShaderProgram();

		/** @brief Returns the location of the given uniform in this shader program
		  * @note Prints an error message to standard error stream if the uniform is not found
			* @param const char* uniformName - name of the uniform to get the location for
		  * @return GLint - location of the given uniform in this shader program
		  */
		GLint getUniformLocation( const char *uniformName );

		/** @brief Returns the index of the given uniform block in this shader program
		  * @note Prints an error message to standard error stream if the uniform block is not found
			* @param const char* uniformBlockName - name of the uniform block to get the index for
		  * @return GLint - index of the given uniform block in this shader program
		  */
		GLint getUniformBlockIndex( const char *uniformBlockName );
		/** @brief Returns the size of the given uniform block in this shader program
		  * @note Prints an error message to standard error stream if the uniform block is not found
			* @param const char* uniformBlockName - name of the uniform block to get the size for
		  * @return GLint - size of the given uniform block in this shader program
		  */
		GLint getUniformBlockSize( const char *uniformBlockName );
		/** @brief Returns an allocated buffer for the given uniform block in this shader program
		  * @note Prints an error message to standard error stream if the uniform block is not found
			* @param const char* uniformBlockName - name of the uniform block to allocate a buffer for
		  * @return GLubyte* - allocated buffer for the given uniform block in this shader program
		  */
		GLubyte* getUniformBlockBuffer( const char *uniformBlockName );
		/** @brief Returns an array of offsets into the buffer for the given uniform block in this shader program
		  * @note Prints an error message to standard error stream if the uniform block is not found
			* @param const char* uniformBlockName - name of the uniform block to return offsets for
		  * @return GLint* - array of offsets for the given uniform block in this shader program
		  */
		GLint* getUniformBlockOffsets( const char *uniformBlockName );
		/** @brief Returns an array of offsets into the buffer for the given uniform block and names in this shader program
		  * @note Prints an error message to standard error stream if the uniform block is not found
			* @param const char* uniformBlockName - name of the uniform block to return offsets for
			* @param const char* names[] - names of the uniform block components to get offsets for
		  * @return GLint* - array of offsets for the given uniform block in this shader program
		  */
		GLint* getUniformBlockOffsets( const char *uniformBlockName, const char *names[] );
		/** @brief Set the binding point for the given uniform block in this shader program
		  * @note Prints an error message to standard error stream if the uniform block is not found
			* @param const char* uniformBlockName - name of the uniform block to bind
			* @param GLuint binding               - binding point for this uniform block
		  */
		void setUniformBlockBinding( const char *uniformBlockName, GLuint binding );

		/** @brief Returns the location of the given attribute in this shader program
		  * @note Prints an error message to standard error stream if the attribute is not found
			* @param const char* attributeName - name of the attribute to get the location for
		  * @return GLint - location of the given attribute in this shader program
		  */
		GLint getAttributeLocation( const char *attributeName );

		/** @brief Returns the index of the given subroutine for a shader stage in this shader program
		  * @note Prints an error message to standard error stream if the subroutine is not found
			* @param GLenum shaderStage         - stage of the shader program to get the subroutine for.
			*   Allowable values: GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER
			* @param const char* subroutineName - name of the subroutine to get the location for
		  * @return GLuint - index of the given subroutine for the shader stage in this shader program
		  */
		GLuint getSubroutineIndex( GLenum shaderStage, const char *subroutineName );

		/** @brief Returns the number of active uniforms in this shader program
		  * @return GLuint - number of active uniforms in this shader program
		  */
		GLuint getNumUniforms();
		/** @brief Returns the number of active uniform blocks in this shader program
		  * @return GLuint - number of active uniform blocks in this shader program
		  */
		GLuint getNumUniformBlocks();
		/** @brief Returns the number of active attributes in this shader program
		  * @return GLuint - number of active attributes in this shader program
		  */
		GLuint getNumAttributes();

		/** @brief Returns the handle for this shader program
		  * @return GLuint - handle for this shader program
		  */
		GLuint getShaderProgramHandle();

        /** @brief Returns the stages held within the shader program
          * @return GLbitfield - the stages for this shader program
          */
        GLbitfield getShaderStages();

		/** @brief Sets the Shader Program to be active
		  */
		void useProgram();
	private:
		ShaderProgram();

		static bool sDEBUG;
        static bool sSEPARABLE;

		GLuint _vertexShaderHandle;
		GLuint _tesselationControlShaderHandle;
		GLuint _tesselationEvaluationShaderHandle;
		GLuint _geometryShaderHandle;
		GLuint _fragmentShaderHandle;
        GLuint _computeShaderHandle;

		GLuint _shaderProgramHandle;
		GLbitfield _stages;

		bool registerShaderProgram( const char *shaderFilenames[], GLbitfield stages);
        bool registerShaderProgram( const char *vertexShaderFilename,
                                    const char *tesselationControlShaderFilename,
                                    const char *tesselationEvaluationShaderFilename,
                                    const char *geometryShaderFilename,
                                    const char *fragmentShaderFilename);

		GLint* getUniformBlockOffsets( GLint uniformBlockIndex );
		GLint* getUniformBlockOffsets( GLint uniformBlockIndex, const char *names[] );
	};

}

////////////////////////////////////////////////////////////////////////////////

bool CSCI444::ShaderProgram::sDEBUG = true;
bool CSCI444::ShaderProgram::sSEPARABLE = false;

void CSCI444::ShaderProgram::enableDebugMessages() {
	sDEBUG = true;
}
void CSCI444::ShaderProgram::disableDebugMessages() {
    sDEBUG = false;
}

void CSCI444::ShaderProgram::enableSeparablePrograms() {
    sSEPARABLE = true;
}
void CSCI444::ShaderProgram::disableSeparablePrograms() {
    sSEPARABLE = false;
}

CSCI444::ShaderProgram::ShaderProgram(const char *vertexShaderFilename, const char *fragmentShaderFilename ) {
	registerShaderProgram( vertexShaderFilename, "", "", "", fragmentShaderFilename );
}

CSCI444::ShaderProgram::ShaderProgram(const char *vertexShaderFilename, const char *tesselationControlShaderFilename, const char *tesselationEvaluationShaderFilename, const char *geometryShaderFilename, const char *fragmentShaderFilename ) {
	registerShaderProgram( vertexShaderFilename, tesselationControlShaderFilename, tesselationEvaluationShaderFilename, geometryShaderFilename, fragmentShaderFilename );
}

CSCI444::ShaderProgram::ShaderProgram(const char *vertexShaderFilename, const char *tesselationControlShaderFilename, const char *tesselationEvaluationShaderFilename, const char *fragmentShaderFilename ) {
	registerShaderProgram( vertexShaderFilename, tesselationControlShaderFilename, tesselationEvaluationShaderFilename, "", fragmentShaderFilename );
}

CSCI444::ShaderProgram::ShaderProgram(const char *vertexShaderFilename, const char *geometryShaderFilename, const char *fragmentShaderFilename ) {
	registerShaderProgram( vertexShaderFilename, "", "", geometryShaderFilename, fragmentShaderFilename );
}

CSCI444::ShaderProgram::ShaderProgram(const char *shaderFilenames[], GLbitfield stages){
    registerShaderProgram(shaderFilenames, stages);
}

bool CSCI444::ShaderProgram::registerShaderProgram(const char *vertexShaderFilename, const char *tesselationControlShaderFilename, const char *tesselationEvaluationShaderFilename, const char *geometryShaderFilename, const char *fragmentShaderFilename ) {
    const char* shaderFilenames[5]; // Store the file names for the program stages
	int counter = 0; // Store the current index to use within the shaderFilenames
    _stages = 0;

    _stages = _stages | GL_VERTEX_SHADER_BIT; // Add stage to program stages
    shaderFilenames[counter] = vertexShaderFilename; // Add filename to filenames
    counter ++;

	if( strcmp( tesselationControlShaderFilename, "" ) != 0 ) {
        _stages = _stages | GL_TESS_CONTROL_SHADER_BIT; // Add stage to program stages
        shaderFilenames[counter] = tesselationControlShaderFilename; // Add filename to filenames
        counter ++;
	}

	if( strcmp( tesselationEvaluationShaderFilename, "" ) != 0 ) {
        _stages = _stages | GL_TESS_EVALUATION_SHADER_BIT; // Add stage to program stages
        shaderFilenames[counter] = tesselationEvaluationShaderFilename; // Add filename to filenames
        counter ++;
	}

	if( strcmp( geometryShaderFilename, "" ) != 0 ) {
        _stages = _stages | GL_GEOMETRY_SHADER_BIT; // Add stage to program stages
        shaderFilenames[counter] = geometryShaderFilename; // Add filename to filenames
        counter ++;
	}

    _stages = _stages | GL_FRAGMENT_SHADER_BIT; // Add stage to program stages
    shaderFilenames[counter] = fragmentShaderFilename; // Add filename to filenames

	return registerShaderProgram(shaderFilenames, _stages);
}

bool CSCI444::ShaderProgram::registerShaderProgram(const char *shaderFilenames[], GLbitfield stages){
    /* set the program stages */
    _stages = stages;

    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    int counter = 0; // Stores the next index to use in shaderFilenames

    if( sDEBUG ) printf( "\n[INFO]: /--------------------------------------------------------\\\n");

    /* compile each one of our shaders */
    /// Vertex Shader
    if((GL_VERTEX_SHADER_BIT & stages) != 0){
        if (sDEBUG) printf("[INFO]: | Vertex Shader: %39s |\n", shaderFilenames[counter]);
        _vertexShaderHandle = CSCI444_INTERNAL::ShaderUtils::compileShader(shaderFilenames[counter], GL_VERTEX_SHADER);
        counter++;
    }else{
        _vertexShaderHandle = 0;
    }

    /// Tesselation Control Shader
    if((GL_TESS_CONTROL_SHADER_BIT & stages) != 0) {
        if (sDEBUG) printf("[INFO]: | Tess Control Shader: %33s |\n", shaderFilenames[counter]);
        if (major < 4) {
            printf("[ERROR]:|   TESSELATION SHADER NOT SUPPORTED!!  UPGRADE TO v4.0+ |\n");
            _tesselationControlShaderHandle = 0;
        } else {
            _tesselationControlShaderHandle = CSCI444_INTERNAL::ShaderUtils::compileShader(
                    shaderFilenames[counter], GL_TESS_CONTROL_SHADER);
        }
        counter ++;
    } else {
        _tesselationControlShaderHandle = 0;
    }

    /// Tesselation Evaluation Shader
    if( (GL_TESS_EVALUATION_SHADER_BIT & stages) != 0) {
        if( sDEBUG ) printf( "[INFO]: | Tess Evaluation Shader: %30s |\n", shaderFilenames[counter] );
        if( major < 4 ) {
            printf( "[ERROR]:|   TESSELATION SHADER NOT SUPPORTED!!  UPGRADE TO v4.0+ |\n" );
            _tesselationEvaluationShaderHandle = 0;
        } else {
            _tesselationEvaluationShaderHandle = CSCI444_INTERNAL::ShaderUtils::compileShader( shaderFilenames[counter], GL_TESS_EVALUATION_SHADER );
        }
        counter++;
    } else {
        _tesselationEvaluationShaderHandle = 0;
    }

    /// Geometry Shader
    if( (GL_GEOMETRY_SHADER_BIT & stages) != 0 != 0 ) {
        if( sDEBUG ) printf( "[INFO]: | Geometry Shader: %37s |\n", shaderFilenames[counter]);
        if( major < 3 || (major == 3 && minor < 2) ) {
            printf( "[ERROR]:|   GEOMETRY SHADER NOT SUPPORTED!!!    UPGRADE TO v3.2+ |\n" );
            _geometryShaderHandle = 0;
        } else {
            _geometryShaderHandle = CSCI444_INTERNAL::ShaderUtils::compileShader( shaderFilenames[counter], GL_GEOMETRY_SHADER );
        }
        counter++;
    } else {
        _geometryShaderHandle = 0;
    }

    /// Fragment Shader
    if((GL_FRAGMENT_SHADER_BIT & stages) != 0) {
        if (sDEBUG) printf("[INFO]: | Fragment Shader: %37s |\n", shaderFilenames[counter]);
        _fragmentShaderHandle = CSCI444_INTERNAL::ShaderUtils::compileShader(shaderFilenames[counter],
                                                                             GL_FRAGMENT_SHADER);
        counter++;
    }else{
        _fragmentShaderHandle = 0;
    }

    /// Compute Shader
    if((GL_COMPUTE_SHADER_BIT & stages) != 0) {
        if( sDEBUG ) printf( "[INFO]: | Compute Shader: %37s |\n", shaderFilenames[counter]);
        if( major < 4 || (major == 4 && minor < 3) ) {
            printf( "[ERROR]:|   COMPUTE SHADER NOT SUPPORTED!!!    UPGRADE TO v4.3+ |\n" );
            _computeShaderHandle = 0;
        } else {
            _computeShaderHandle = CSCI444_INTERNAL::ShaderUtils::compileShader( shaderFilenames[counter], GL_COMPUTE_SHADER );
        }
        counter++;
    }else{
        _computeShaderHandle = 0;
    }

    /* get a handle to a shader program */
    _shaderProgramHandle = glCreateProgram();
    glProgramParameteri(_shaderProgramHandle, GL_PROGRAM_SEPARABLE, sSEPARABLE);

    /* attach the vertex and fragment shaders to the shader program */
    if(_vertexShaderHandle != 0) {
        glAttachShader(_shaderProgramHandle, _vertexShaderHandle);
    }
    if( _tesselationControlShaderHandle != 0 ) {
        glAttachShader( _shaderProgramHandle, _tesselationControlShaderHandle );
    }
    if( _tesselationEvaluationShaderHandle != 0 ) {
        glAttachShader( _shaderProgramHandle, _tesselationEvaluationShaderHandle );
    }
    if( _geometryShaderHandle != 0 ) {
        glAttachShader( _shaderProgramHandle, _geometryShaderHandle );
    }
    if(_fragmentShaderHandle != 0) {
        glAttachShader(_shaderProgramHandle, _fragmentShaderHandle);
    }
    if(_computeShaderHandle != 0) {
        glAttachShader(_shaderProgramHandle, _computeShaderHandle);
    }

    /* link all the programs together on the GPU */
    glLinkProgram( _shaderProgramHandle );

    if( sDEBUG ) printf( "[INFO]: | Shader Program: %41s", "|\n" );

    /* check the program log */
    CSCI444_INTERNAL::ShaderUtils::printLog( _shaderProgramHandle );

    GLint separable = GL_FALSE;
    glGetProgramiv( _shaderProgramHandle, GL_PROGRAM_SEPARABLE, &separable );

    if( sDEBUG ) printf( "[INFO]: | Program Separable: %35s |\n", (separable ? "Yes" : "No"));

    /* print shader info for uniforms & attributes */
    CSCI444_INTERNAL::ShaderUtils::printShaderProgramInfo( _shaderProgramHandle );

    /* return handle */
    return _shaderProgramHandle != 0;
}

GLint CSCI444::ShaderProgram::getUniformLocation(const char *uniformName ) {
	GLint uniformLoc = glGetUniformLocation( _shaderProgramHandle, uniformName );
	if( uniformLoc == -1 )
		fprintf( stderr, "[ERROR]: Could not find uniform %s\n", uniformName );
	return uniformLoc;
}

GLint CSCI444::ShaderProgram::getUniformBlockIndex(const char *uniformBlockName ) {
	GLint uniformBlockLoc = glGetUniformBlockIndex( _shaderProgramHandle, uniformBlockName );
	if( uniformBlockLoc == -1 )
		fprintf( stderr, "[ERROR]: Could not find uniform block %s\n", uniformBlockName );
	return uniformBlockLoc;
}

GLint CSCI444::ShaderProgram::getUniformBlockSize(const char *uniformBlockName ) {
	GLint blockSize;
	glGetActiveUniformBlockiv( _shaderProgramHandle, getUniformBlockIndex(uniformBlockName), GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize );
	return blockSize;
}

GLubyte* CSCI444::ShaderProgram::getUniformBlockBuffer(const char *uniformBlockName ) {
	GLubyte *blockBuffer;

	GLint blockSize = getUniformBlockSize( uniformBlockName );

	blockBuffer = (GLubyte*)malloc(blockSize);

	return blockBuffer;
}

GLint* CSCI444::ShaderProgram::getUniformBlockOffsets(const char *uniformBlockName ) {
	return getUniformBlockOffsets( getUniformBlockIndex(uniformBlockName) );
}

GLint* CSCI444::ShaderProgram::getUniformBlockOffsets(const char *uniformBlockName, const char *names[] ) {
	return getUniformBlockOffsets( getUniformBlockIndex(uniformBlockName), names );
}

GLint* CSCI444::ShaderProgram::getUniformBlockOffsets(GLint uniformBlockIndex ) {
	GLint numUniforms;
	glGetActiveUniformBlockiv( _shaderProgramHandle, uniformBlockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numUniforms );

	GLuint *indices = (GLuint*)malloc(numUniforms*sizeof(GLuint));
	glGetActiveUniformBlockiv( _shaderProgramHandle, uniformBlockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, (GLint*)indices);

	GLint *offsets = (GLint*)malloc(numUniforms*sizeof(GLint));
	glGetActiveUniformsiv( _shaderProgramHandle, numUniforms, indices, GL_UNIFORM_OFFSET, offsets );
	return offsets;
}

GLint* CSCI444::ShaderProgram::getUniformBlockOffsets(GLint uniformBlockIndex, const char *names[] ) {
	GLint numUniforms;
	glGetActiveUniformBlockiv( _shaderProgramHandle, uniformBlockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numUniforms );

	GLuint *indices = (GLuint*)malloc(numUniforms*sizeof(GLuint));
	glGetUniformIndices( _shaderProgramHandle, numUniforms, names, indices );

	GLint *offsets = (GLint*)malloc(numUniforms*sizeof(GLint));
	glGetActiveUniformsiv( _shaderProgramHandle, numUniforms, indices, GL_UNIFORM_OFFSET, offsets );
	return offsets;
}

void CSCI444::ShaderProgram::setUniformBlockBinding(const char *uniformBlockName, GLuint binding ) {
	glUniformBlockBinding( _shaderProgramHandle, getUniformBlockIndex(uniformBlockName), binding );
}

GLint CSCI444::ShaderProgram::getAttributeLocation(const char *attributeName ) {
	GLint attributeLoc = glGetAttribLocation( _shaderProgramHandle, attributeName );
	if( attributeLoc == -1 )
		fprintf( stderr, "[ERROR]: Could not find attribute %s\n", attributeName );
	return attributeLoc;
}

GLuint CSCI444::ShaderProgram::getSubroutineIndex(GLenum shaderStage, const char *subroutineName ) {
	GLuint subroutineIndex = glGetSubroutineIndex( _shaderProgramHandle, shaderStage, subroutineName );
	if( subroutineIndex == GL_INVALID_INDEX )
		fprintf( stderr, "[ERROR]: Could not find subroutine %s for %s\n", subroutineName, CSCI444_INTERNAL::ShaderUtils::GL_shader_type_to_string(shaderStage) );
	return subroutineIndex;
}

GLuint CSCI444::ShaderProgram::getNumUniforms() {
	int numUniform = 0;
	glGetProgramiv( _shaderProgramHandle, GL_ACTIVE_UNIFORMS, &numUniform );
	return numUniform;
}

GLuint CSCI444::ShaderProgram::getNumUniformBlocks() {
	int numUniformBlocks = 0;
	glGetProgramiv( _shaderProgramHandle, GL_ACTIVE_UNIFORM_BLOCKS, &numUniformBlocks );
	return numUniformBlocks;
}

GLuint CSCI444::ShaderProgram::getNumAttributes() {
	int numAttr = 0;
	glGetProgramiv( _shaderProgramHandle, GL_ACTIVE_ATTRIBUTES, &numAttr );
	return numAttr;
}

GLuint CSCI444::ShaderProgram::getShaderProgramHandle() {
	return _shaderProgramHandle;
}

GLbitfield CSCI444::ShaderProgram::getShaderStages(){
    return _stages;
}

void CSCI444::ShaderProgram::useProgram() {
	glUseProgram( _shaderProgramHandle );
}

CSCI444::ShaderProgram::ShaderProgram() {}

CSCI444::ShaderProgram::~ShaderProgram() {
	glDeleteShader( _vertexShaderHandle );
	glDeleteShader( _tesselationControlShaderHandle );
	glDeleteShader( _tesselationEvaluationShaderHandle );
	glDeleteShader( _geometryShaderHandle );
	glDeleteShader( _fragmentShaderHandle );
	glDeleteProgram( _shaderProgramHandle );
}

#endif //__CSCI444_SHADEREPROGRAM_4_H__
