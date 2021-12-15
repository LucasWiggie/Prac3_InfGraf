#include "BOX.h"
#include "auxiliar.h"


#include <gl/glew.h>
#define SOLVE_FGLUT_WARNING
#include <gl/freeglut.h> 

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#define PI 3.1415926535897932384626433832795


//////////////////////////////////////////////////////////////
// Datos que se almacenan en la memoria de la CPU
//////////////////////////////////////////////////////////////

//Matrices
glm::mat4	proj = glm::mat4(1.0f);
glm::mat4	view = glm::mat4(1.0f);
glm::mat4	model = glm::mat4(1.0f);
glm::mat4	model2 = glm::mat4(1.0f);

//Variables de la Luz
glm::vec3 lightPosition = glm::vec3(0.0, 0.0, 0.0);
glm::vec3 lightIntensity = glm::vec3(1.0);

//////////////////////////////////////////////////////////////
// Variables que nos dan acceso a Objetos OpenGL ( VARIABLES GLOBALES )
//////////////////////////////////////////////////////////////

// Variables que nos permiten identificar y manejar el shader de vertices, fragmentos y el programa que los enlaza
unsigned int vshader;
unsigned int fshader;
unsigned int program;

// Variables globales que nos permiten acceder a las variables uniformes y a los atributos
// Variables Uniform
int uView;
int uModelViewMat;
int uModelViewProjMat;
int uNormalMat;
int uColorTex;
int uEmiTex;
int uLightPosition;
int uLightIntensity;

//Atributos
int inPos;
int inColor;
int inNormal;
int inTexCoord;

//VAO Vertex Array Object
unsigned int vao;

//VBOs Vertex Buffer Objects que forman parte del objeto
// Almacena una gran cantidad de vértices en la memoria de la GPU
unsigned int posVBO;
unsigned int colorVBO;
unsigned int normalVBO;
unsigned int texCoordVBO;
unsigned int triangleIndexVBO;

//Texturas
unsigned int colorTexId;
unsigned int emiTexId;

//////////////////////////////////////////////////////////////
// Funciones auxiliares
//////////////////////////////////////////////////////////////
//!!Por implementar

//Declaración de CB
void renderFunc();
void resizeFunc(int width, int height);
void idleFunc();
void keyboardFunc(unsigned char key, int x, int y);
void mouseFunc(int button, int state, int x, int y);

//Funciones de inicialización y destrucción
void initContext(int argc, char** argv);
void initOGL();
void initShader(const char *vname, const char *fname);
void initObj();
void destroy();


//Carga el shader indicado, devuele el ID del shader
//!Por implementar
GLuint loadShader(const char *fileName, GLenum type);

//Crea una textura, la configura, la sube a OpenGL, 
//y devuelve el identificador de la textura 
//!!Por implementar
unsigned int loadTex(const char *fileName);


int main(int argc, char** argv)
{
	std::locale::global(std::locale("spanish"));// acentos ;)

	initContext(argc, argv);
	initOGL();
	initShader("../shaders_P3/shader.Ej1.vert", "../shaders_P3/shader.Ej1.frag");
	initObj();

	//Bucle de dibujado. Lo inicializamos después de haber inicializado el cauce y el contexto (todo lo de arriba)
	glutMainLoop();
	destroy();

	return 0;
}
	
//////////////////////////////////////////
// Funciones auxiliares 

// Creación del contexto Open GL
void initContext(int argc, char** argv){
	
	// Creamos el contexto utilizando GLUT
	glutInit(&argc, argv);
	glutInitContextVersion(3, 3); //version OpenGL 3.3
	glutInitContextProfile(GLUT_CORE_PROFILE); 

	// Definimos el Frame Buffer y creamos la ventana de visualización
	// (Tipo de Buffer doble lienzo/pantalla | Color RGBA | Crea un mapa de profundidad)
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); //define el Frame Buffer
	glutInitWindowSize(500, 500); //establecemos el tamaño de la ventana
	glutInitWindowPosition(0, 0); // establecemos la posicion de la ventana
	glutCreateWindow("Práctica OGL"); //creamos la ventana

	// Inicializamos las Extensiones
	GLenum err = glewInit(); //identificador de todos los tipos de valores que puede devolver glewInit(), la inicializacion del contexto
	if (GLEW_OK != err) //GLEW_OK = 0, si ha habido un problema, err != 0
	{ 
		std::cout << "Error: " << glewGetErrorString(err) << std::endl; //leemos el error y lo mostarmos por pantalla
		exit(-1);
	}
	const GLubyte* oglVersion = glGetString(GL_VERSION); //comprobamos que la version de OpenGL es soportada por el sistema
	std::cout << "This system supports OpenGL Version: " << oglVersion << std::endl;

	//Asignamos qué funciones tratarán distintos eventos
	glutReshapeFunc(resizeFunc);
	glutDisplayFunc(renderFunc);
	glutIdleFunc(idleFunc);
	glutKeyboardFunc(keyboardFunc);
	glutMouseFunc(mouseFunc);

}

// Configuración del cauce
void initOGL(){

	// Buffer de profundidad y color de fondo
	glEnable(GL_DEPTH_TEST); //habilita el bufer de profundidad
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f); //establce el color de fondo

	// Configuración de las caras
	glFrontFace(GL_CCW); //establce la cara frontal, counter clock wise
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //se generan fragmentos en la cara de delante y detrás
	glEnable(GL_CULL_FACE); //activamos culling, no se calculan las caras que no se vean desde la camara

	// Definimos las matrices de vista y proyección
	proj = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 50.0f);
	view = glm::mat4(1.0f);
	view[3].z = -6;
}

void destroy(){

	// Liberamos el espacio de memoria de los shaders
	glDetachShader(program, vshader);
	glDetachShader(program, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);
	glDeleteProgram(program);

	// Libera el espacio utilizado para los recursos del objeto
	if (inPos != -1) glDeleteBuffers(1, &posVBO);
	if (inColor != -1) glDeleteBuffers(1, &colorVBO);
	if (inNormal != -1) glDeleteBuffers(1, &normalVBO);
	if (inTexCoord != -1) glDeleteBuffers(1, &texCoordVBO);
	glDeleteBuffers(1, &triangleIndexVBO);
	
	glDeleteVertexArrays(1, &vao);

	// Texturas
	glDeleteTextures(1, &colorTexId);
	glDeleteTextures(1, &emiTexId);

}

void initShader(const char *vname, const char *fname){
	
	// Cargamos el shader de vertices y de fragmentos con loadShader()
	vshader = loadShader(vname, GL_VERTEX_SHADER); 
	fshader = loadShader(fname, GL_FRAGMENT_SHADER);

	// Creamos un programa y enlazamos los shaders al programa
	program = glCreateProgram();
	glAttachShader(program, vshader);
	glAttachShader(program, fshader);

	// Asigna un identificador y posición de memoria a los atributos del programa (antes de enlazar el programa)
	glBindAttribLocation(program, 0, "inPos");
	glBindAttribLocation(program, 1, "inColor");
	glBindAttribLocation(program, 2, "inNormal");
	glBindAttribLocation(program, 3, "inTexCoord");

	// Linkea el programa
	glLinkProgram(program);

	// Comprobación de si ha habido error en la fase de enlazado
	int linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) //si ha habdio un problema de compilación, linked = false 
	{
		//Calculamos una cadena de error
		GLint logLen;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);  //le asigna la cadena de error a logLen

		char* logString = new char[logLen]; // reservamos memoria para logString, de tamaño logLen
		glGetProgramInfoLog(program, logLen, NULL, logString); //pedimos la info del error y la almacenamos en logString
		std::cout << "Error: " << logString << std::endl;

		delete[] logString; //vaciamos la memoria de logString
		glDeleteProgram(program); //borramos el programa 
		program = 0;
		exit(-1);
	}

	// Crea los identificadores de las variables uniformes 
	uNormalMat = glGetUniformLocation(program, "normal");
	uModelViewMat = glGetUniformLocation(program, "modelView");
	uModelViewProjMat = glGetUniformLocation(program, "modelViewProj");
	uView = glGetUniformLocation(program, "view");
	// Identificadores de los atributos de la luz 
	uLightPosition = glGetUniformLocation(program, "lightPos");
	uLightIntensity = glGetUniformLocation(program, "lightInt");

	// Identificadores de las texturas
	uColorTex = glGetUniformLocation(program, "colorTex");
	uEmiTex = glGetUniformLocation(program, "emiTex");

	// Crea los identificadores de los atributos
	inPos = glGetAttribLocation(program, "inPos");
	inColor = glGetAttribLocation(program, "inColor");
	inNormal = glGetAttribLocation(program, "inNormal");
	inTexCoord = glGetAttribLocation(program, "inTexCoord");

}

void initObj(){

	// Crea y activa el VAO en el que se almacenará la configuración del objeto
	glGenVertexArrays(1, &vao); //genera el vao
	glBindVertexArray(vao); //asignamos el vertex array al vao, lo activamos

	// Crea y configura todos los atributos de la malla
	if (inPos != -1)
	{ 
		glGenBuffers(1, &posVBO); //genera un buffer
		glBindBuffer(GL_ARRAY_BUFFER, posVBO); //vincula el buffer a la variable posVBO
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex * sizeof(float) * 3,
			cubeVertexPos, GL_STATIC_DRAW); 
		glVertexAttribPointer(inPos, 3, GL_FLOAT, GL_FALSE, 0, 0); // una vez creamos el buffer, definimos el puntero al atributo inPos
		// define an array of generic vertex attribute data
		glEnableVertexAttribArray(inPos); // Activamos el puntero
	}
	if (inColor != -1)
	{
		glGenBuffers(1, &colorVBO);
		glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex * sizeof(float) * 3,
			cubeVertexColor, GL_STATIC_DRAW);
		glVertexAttribPointer(inColor, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inColor);
	}
	if (inNormal != -1)
	{
		glGenBuffers(1, &normalVBO);
		glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex * sizeof(float) * 3,
			cubeVertexNormal, GL_STATIC_DRAW);
		glVertexAttribPointer(inNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inNormal);
	}
	if (inTexCoord != -1)
	{
		glGenBuffers(1, &texCoordVBO);
		glBindBuffer(GL_ARRAY_BUFFER, texCoordVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex * sizeof(float) * 2,
			cubeVertexTexCoord, GL_STATIC_DRAW);
		glVertexAttribPointer(inTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inTexCoord);
	}

	// Crea la lista de indices
	glGenBuffers(1, &triangleIndexVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangleIndexVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		cubeNTriangleIndex * sizeof(unsigned int) * 3, cubeTriangleIndex,
		GL_STATIC_DRAW);

	// Inicializa la matriz model de ese objeto
	model = glm::mat4(1.0f);
	model2 = glm::mat4(1.0f);

	// Cargamos las texturas
	colorTexId = loadTex("../img/color2.png");
	emiTexId = loadTex("../img/emissive.png");

}

GLuint loadShader(const char *fileName, GLenum type){
	
	// Carga de un shader genérico
	unsigned int fileLen; //contador para ver la longitud del fichero
	char* source = loadStringFromFile(fileName, fileLen); //pasamos el nombre del fichero que vamos a cargar

	// Compilación del shader
	GLuint shader; //unsigned int
	shader = glCreateShader(type); //creación del shader, leyendo el tipo de GLenum que s enos pasa como argumento en el método
	glShaderSource(shader, 1,
		(const GLchar**)&source, (const GLint*)&fileLen); // cargamos el codigo basico del shader
	glCompileShader(shader); //utilizamos esta función auxiliar para compilarlo
	delete[] source; //borramos en array porque LoadStringFromFile reserva memoria en forma de array

	// Comprobamos que se ha compilado bien
	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled) { //si la compilación ha ido mal, compiled = false
		
		//Calculamos una cadena de error
		GLint logLen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen); //le asigna la cadena de error a logLen

		char* logString = new char[logLen]; // reservamos memoria para logString, de tamaño logLen
		glGetShaderInfoLog(shader, logLen, NULL, logString); //pedimos la info de error y la almacenamos en logString
		std::cout << "Error: " << logString << std::endl;

		delete[] logString; //vaciamos la memoria de logString
		glDeleteShader(shader); //borramos el shader
		exit(-1);

	}

	return shader; 
}

unsigned int loadTex(const char *fileName){ 
	
	//Cargar y configurar una textura genérica.
	
	//Carga la textura almacenada en el fichero indicado
	unsigned char *map;
	unsigned int w, h;
	map = loadTexture(fileName, w, h); //función auxiliar de auxiliar.cpp

	if (!map) {
		std::cout << "Error cargando el fichero: " << fileName << std::endl;
		exit(-1);
	}

	//Crea una textura, actívala y súbela a la tarjeta gráfica
	unsigned int texId;
	glGenTextures(1, &texId); 
	glBindTexture(GL_TEXTURE_2D, texId); //vincula la textura al buffer y asigna un id nuevo
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) map); //subimos la textura  a la tarjeta grafica
	glGenerateMipmap(GL_TEXTURE_2D); //generamos mip_map

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);

	delete[] map;

	return texId; 

}

void resizeFunc(int width, int height) {
	
	glViewport(0, 0, width, height); //ajustamos el viewport con la altura y anchura dadas

	float aspectRatio = float(width) / float(height);

	float f = 1.0f / tan(PI / 4);
	float farPlane = 10.0;
	float nearPlane = 0.1;

	proj[0].x = 1.0 / (aspectRatio * (tan(PI / 4)));
	proj[1].y = f;
	proj[2].z = (farPlane + nearPlane) / (nearPlane - farPlane);
	proj[2].w = -1.0f;
	proj[3].z = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
	proj[3].w = 0.0f;

	glutPostRedisplay(); //Lanza un evento de renderizado, marks the current window as needing to be redisplayed.
}

void renderFunc(){
	
	// Limpiamos el buffer de color y el de profundidad antes de cada renderizado
	// Lo primero que haces es limpiar el renderizado anterior, antes de empezar uno nuevo
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Activa el programa que hemos definido para usar los sahders
	glUseProgram(program);

	// Calcula y sube las matrices requeridas por el shader de vertices
	glm::mat4 modelView = view * model;
	glm::mat4 modelViewProj = proj * view * model;
	glm::mat4 normal = glm::transpose(glm::inverse(modelView));

	//Utilizamos los identificadores creados en InitShader() que iddentifican las matrices dentro del shader, las subimos 
	if (uModelViewMat != -1) //comprobamos que esa matriz está, (su id será -1 si el shader no al encuentra, es el valor por defecto)
		glUniformMatrix4fv(uModelViewMat, 1, GL_FALSE,
			&(modelView[0][0]));
	if (uModelViewProjMat != -1)
		glUniformMatrix4fv(uModelViewProjMat, 1, GL_FALSE,
			&(modelViewProj[0][0]));
	if (uView != -1)
		glUniformMatrix4fv(uView, 1, GL_FALSE,
			&(view[0][0]));
	if (uNormalMat != -1)
		glUniformMatrix4fv(uNormalMat, 1, GL_FALSE,
			&(normal[0][0]));
	if (uLightPosition != -1)
		glUniformMatrix4fv(uLightPosition, 1, GL_FALSE,
			glm::value_ptr(lightPosition));
	if (uLightIntensity != -1)
		glUniformMatrix4fv(uLightIntensity, 1, GL_FALSE,
			glm::value_ptr(lightIntensity));

	//Texturas
	if (uColorTex != -1)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorTexId);
		glUniform1i(uColorTex, 0);
	}
	if (uEmiTex != -1)
	{
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, emiTexId);
		glUniform1i(uEmiTex, 1);
	}

	//Activa el VAO con la configuración del objeto y pinta la lista de triángulos
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, cubeNTriangleIndex * 3,
		GL_UNSIGNED_INT, (void*)0);

	// Después de haber pintado en el default frame bufefr (lienzo), debemos pasarlo al bufer de pantalla
	glutSwapBuffers(); 
}

void idleFunc(){

	// Objeto 1
	//Animacion del cubo girando con rotacion
	model = glm::mat4(1.0f);
	static float angle = 0.0f;
	angle = (angle > 3.141592f * 2.0f) ? 0 : angle + 0.0001f;

	model = glm::rotate(model, angle, glm::vec3(1.0f, 1.0f, 0.0f));

	// Objeto 2
	model2 = glm::mat4(1.0f);
	static float r_angle = 0.0f;
	r_angle = (r_angle > 3.141592f * 2.0f) ? 0 : r_angle + 0.0001f;
	//Rotación sobre si mismo
	model2 = glm::rotate(model2, angle, glm::vec3(1.0f, 1.0f, 0.0f));
	//Traslacion 
	model2 = glm::translate(model2, glm::vec3(0.5, 0.0, 0.0));
	//Rotacion alrededor del otro objeto
	model2 = glm::rotate(model2, angle, glm::vec3(0.0f, 1.0f, 0.0f));

	glutPostRedisplay();
}

void keyboardFunc(unsigned char key, int x, int y){

	std::cout << "Se ha pulsado la tecla " << key << std::endl << std::endl;

	// Cada vez que movemos la camara, hay que recalcular el frustrum
	float f = 1.0f / tan(3.1415926535897932384626433832795 / 4);
	float farPlane = 10.0;
	float nearPlane = 0.1;
	static float r_angle = 0.0;

	proj[0].x = f;
	proj[1].y = f;
	proj[2].z = (farPlane + nearPlane) / (nearPlane - farPlane);
	proj[2].w = -1.0f;
	proj[3].z = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
	proj[3].w = 0.0f;

	//Coordenadas de posicion de la camara
	//glm::mat4 view = glm::mat4(1.0);
	static float movementX = 0.0;
	static float movementZ = -6.0;
	//static float lightMovementX = 0.0;
	//static float lightMovementZ = 0.5;
	static glm::vec3 lightIntensityVar = glm::vec3(0.0);

	switch (key) {
	case 'w':
		movementZ = movementZ + 0.2;
		break;
	case 's':
		movementZ = movementZ - 0.2;
		break;
	case 'a':
		movementX = movementX + 0.2;
		break;
	case 'd':
		movementX = movementX - 0.2;
		break;
	case 'q':
		r_angle = (r_angle < 3.141599 * 2.0f) ? r_angle - 0.01f : 0.0f;
		break;
	case 'e':
		r_angle = (r_angle < 3.141599 * 2.0f) ? r_angle + 0.01f : 0.0f;
		break;
	case 'i':
		lightPosition.z += 0.2;
		break;
	case 'k':
		lightPosition.z -= 0.2;
		break;
	case 'l':
		lightPosition.x-= 0.2;
		break;
	case 'j':
		lightPosition.x += 0.2;
		break;
	case 'u':
		lightIntensityVar -= glm::vec3(0.1, 0.1, 0.1);
		/*if (lightIntensity.x > 0 && lightIntensity.x < 1) {
			lightIntensity -= glm::vec3(0.1, 0.1, 0.1);
		}*/
		break;
	case 'o':
		lightIntensityVar += glm::vec3(0.1, 0.1, 0.1);
		/*if (lightIntensity.x > 0 && lightIntensity.x < 1) {
			lightIntensity += glm::vec3(0.1, 0.1, 0.1);
		}*/
		break;
	}

	// Cámara
	view[3].z = movementZ;
	view[3].x = movementX;
	proj = glm::rotate(proj, r_angle, glm::vec3(0.0f, 1.0f, 0.0f));
	view = glm::rotate(view, r_angle, glm::vec3(0.0f, 1.0f, 0.0f));
	
	lightIntensity += lightIntensityVar;
	/*lightPosition.z = lightMovementZ;
	lightPosition.x = lightMovementX;*/

	lightPosition = glm::vec3(view * glm::vec4(lightPosition, 1.0f));
	glutPostRedisplay();
}

void mouseFunc(int button, int state, int x, int y){}









