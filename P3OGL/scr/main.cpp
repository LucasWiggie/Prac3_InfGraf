#include "BOX.h"
#include "auxiliar.h"


#include <gl/glew.h>
#define SOLVE_FGLUT_WARNING
#include <gl/freeglut.h> 

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>


//////////////////////////////////////////////////////////////
// Datos que se almacenan en la memoria de la CPU
//////////////////////////////////////////////////////////////

//Matrices
glm::mat4	proj = glm::mat4(1.0f);
glm::mat4	view = glm::mat4(1.0f);
glm::mat4	model = glm::mat4(1.0f);


//////////////////////////////////////////////////////////////
// Variables que nos dan acceso a Objetos OpenGL (VARIABLES GLOBALES )
//////////////////////////////////////////////////////////////

//Crea tres variables globales que nos permitan manejar el shader del vértices, el shader
//de fragmentos y el programa que los enlaza. Les da un identificador:
unsigned int vshader; 
unsigned int fshader;
unsigned int program;

//Variables Uniform 
int uModelViewMat;
int uModelViewProjMat;
int uNormalMat;
//Texturas Uniform
int uColorTex;
int uEmiTex;


//Atributos
int inPos;
int inColor;
int inNormal;
int inTexCoord;

//VAO (MIRAR QUE SIGINIFICA)
unsigned int vao;

//VBOs que forman parte del objeto
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
	initShader("../shaders_P3/shader.v1.vert", "../shaders_P3/shader.v1.frag");
	initObj();

	// Main Loop, cargamos el bucle de dibujado.
	//Se hace después de haber creado todo lo de arriba
	glutMainLoop();

	destroy();

	return 0;
}
	
//////////////////////////////////////////
// Funciones auxiliares 
void initContext(int argc, char** argv){
	glutInit(&argc, argv);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	//Inicializar Frame Buffer y la ventana de renderizado
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // (tipo de buffer de imagen y de color) modo buffer doble (lienzo-pantalla), con rgba y tambien crea un buffer de profundidad (crea un mapa de profundidad)
	glutInitWindowSize(500, 500); // establecemos reoslucion ventana
	glutInitWindowPosition(0, 0); // posiciuon de donde aparece la ventana
	glutCreateWindow("Prácticas OGL"); // Titulo que le ponemos a la ventana

	// Inicializa Extensiones
	GLenum err = glewInit(); // un identificador d etgodos los tipos de valores q te puede dar de vuelva glesInit()
	if (GLEW_OK != err) // (GLEW_OK = 0) si no se corresponde con el identificador, ha habido un problema
	{
		std::cout << "Error: " << glewGetErrorString(err) << std::endl; // leemos qué error ha sido
		exit(-1);
	}
	const GLubyte* oglVersion = glGetString(GL_VERSION); // comprobamos la version de Open GL
	std::cout << "This system supports OpenGL Version: " << oglVersion << std::endl;

	// Eventos ( funciones callback ), funciones ya definidas ya que estan en amarillo, declaradas por la libreria glut 
	glutReshapeFunc(resizeFunc); 
	glutDisplayFunc(renderFunc);
	glutIdleFunc(idleFunc);
	glutKeyboardFunc(keyboardFunc);
	glutMouseFunc(mouseFunc);

}

void initOGL(){
	glEnable(GL_DEPTH_TEST); // comprueba la profundidad de los fragments y los guarda en el GL_DEPTH (buffer d eprofundidad)
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f); 

	glFrontFace(GL_CCW); // cuando cargues poligonos, dados por vertices, decides la cara frontal
	// CCW: vertices definidos counter clock wise
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // segun esto se van a generar fragmentos en la cara d ealante y en la de atras
	glEnable(GL_CULL_FACE); // estamos diciendo que no hace falta calcular una de las caras (mirar culling)

	//Matriz de perspectiva (hacemos trampas con glm)
	proj = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 50.0f); // 60º de apertura, near, far (creo)
	view = glm::mat4(1.0f); 
	view[3].z = -6; //desplaza el mundo entero seis unidades atrás, no mueves la cámara

}

void destroy(){
	glDetachShader(program, vshader);
	glDetachShader(program, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);
	glDeleteProgram(program);

	//Para eliminar los buffers
	if (inPos != -1) glDeleteBuffers(1, &posVBO);
	if (inColor != -1) glDeleteBuffers(1, &colorVBO);
	if (inNormal != -1) glDeleteBuffers(1, &normalVBO);
	if (inTexCoord != -1) glDeleteBuffers(1, &texCoordVBO);
	glDeleteBuffers(1, &triangleIndexVBO);

	glDeleteVertexArrays(1, &vao);

	//Para eliminar las texturas
	glDeleteTextures(1, &colorTexId);
	glDeleteTextures(1, &emiTexId);

}

void initShader(const char *vname, const char *fname) {
	vshader = loadShader(vname, GL_VERTEX_SHADER); // cargamos el shader de vertices
	fshader = loadShader(fname, GL_FRAGMENT_SHADER); // cargamos el shader de fragmentos

	// enlaza los dos shaders a un programa
	program = glCreateProgram(); // creas el programa
	glAttachShader(program, vshader); // enlazas el shader de vertices al programa
	glAttachShader(program, fshader); // enlazas el shader de fragmentos al programa
	
	// Les asignamos a los atributos un identificador y una posicion de memoria en el programa 
	glBindAttribLocation(program, 0, "inPos");
	glBindAttribLocation(program, 1, "inColor");
	glBindAttribLocation(program, 2, "inNormal");
	glBindAttribLocation(program, 3, "inTexCoord");

	glLinkProgram(program);

	int linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) // si ha habido un problema, linked = 0
	{
		// IGUAL QUE EN LOAD SHADER
		//Calculamos una cadena de error
		GLint logLen;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
		char* logString = new char[logLen];
		glGetProgramInfoLog(program, logLen, NULL, logString);
		std::cout << "Error: " << logString << std::endl;
		delete[] logString;
		glDeleteProgram(program);
		program = 0;
		exit(-1);
	}

	// Después de linkearlo, pedimos al programa qué identificadores ha linkeado
	uNormalMat = glGetUniformLocation(program, "normal");
	uModelViewMat = glGetUniformLocation(program, "modelView");
	uModelViewProjMat = glGetUniformLocation(program, "modelViewProj");

	//Le pedimos al shader el identificador que tiene el uniform (la textura) en él
	uColorTex = glGetUniformLocation(program, "colorTex");
	uEmiTex = glGetUniformLocation(program, "emiTex");

	inPos = glGetAttribLocation(program, "inPos");
	inColor = glGetAttribLocation(program, "inColor");
	inNormal = glGetAttribLocation(program, "inNormal");
	inTexCoord = glGetAttribLocation(program, "inTexCoord");


}

void initObj(){
	glGenVertexArrays(1, &vao); //crea el VAO
	glBindVertexArray(vao); // activa el VAO

	// Crea y configura todos los atributos de la malla
	if (inPos != -1)
	{
		glGenBuffers(1, &posVBO); //genero el Buffer
		glBindBuffer(GL_ARRAY_BUFFER, posVBO); //vinculo el Buffer a posVBO
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex * sizeof(float) * 3, //3 porq tiene XYZ
			cubeVertexPos, GL_STATIC_DRAW);
		glVertexAttribPointer(inPos, 3, GL_FLOAT, GL_FALSE, 0, 0); // una vez creamos el buffer, definimos el puntero al atributo inPos **MIRAR EN KHRONOS
		glEnableVertexAttribArray(inPos); //Activas el puntero
	}
	if (inColor != -1)
	{
		glGenBuffers(1, &colorVBO);
		glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex * sizeof(float) * 3, // 3 porque RGB
			cubeVertexColor, GL_STATIC_DRAW);
		glVertexAttribPointer(inColor, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inColor);
	}
	if (inNormal != -1)
	{
		glGenBuffers(1, &normalVBO);
		glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex * sizeof(float) * 3, //XYZ
			cubeVertexNormal, GL_STATIC_DRAW);
		glVertexAttribPointer(inNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inNormal);
	}
	if (inTexCoord != -1)
	{
		glGenBuffers(1, &texCoordVBO);
		glBindBuffer(GL_ARRAY_BUFFER, texCoordVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex * sizeof(float) * 2, //XY
			cubeVertexTexCoord, GL_STATIC_DRAW);
		glVertexAttribPointer(inTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inTexCoord);
	}

	glGenBuffers(1, &triangleIndexVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangleIndexVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		cubeNTriangleIndex * sizeof(unsigned int) * 3, cubeTriangleIndex,
		GL_STATIC_DRAW);

	model = glm::mat4(1.0f);

	//Cargamos las texturas (si tuvieramos muchos objs en escena merece la pena guardarlo en un array)
	colorTexId = loadTex("../img/color2.png");
	emiTexId = loadTex("../img/emissive.png");

}

GLuint loadShader(const char *fileName, GLenum type){ 
	unsigned int fileLen; //contador para ver la longitud del fichero
	char* source = loadStringFromFile(fileName, fileLen); //pasar al nombre del fichero que vamos a cargar
	
	//////////////////////////////////////////////
	//Creación y compilación del Shader
	GLuint shader;
	shader = glCreateShader(type); //creamos el shader
	glShaderSource(shader, 1,
		(const GLchar**)&source, (const GLint*)&fileLen); // cargamos el codigo basico del shader
	glCompileShader(shader); //le pedimos con esta funcion auxiliar que loi compile
	delete[] source; // borramos (borrado en array porque loadStringFromFile reserva memoria en forma de array)
	
	//Comprobamos que se compiló bien
	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled); //si la compilacion ha ido mal, compiled = 0
	if (!compiled) //si ha habido un problema en la compilacion
	{
		//Calculamos una cadena de error
		GLint logLen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
		
		char* logString = new char[logLen]; // reservamos memoria para logLen
		glGetShaderInfoLog(shader, logLen, NULL, logString); // pedimos la info del error y la almacenamos en logString
		std::cout << "Error: " << logString << std::endl;

		delete[] logString; // borramos y vaciamos la memoria de LogString
		glDeleteShader(shader); // borramos el shader
		exit(-1);
	}

	return shader; // se ha quedado en la memoria de la gpu con un "dni", lo devolvemos
}

unsigned int loadTex(const char *fileName){ 
	
	unsigned char* map;
	unsigned int w, h;
	map = loadTexture(fileName, w, h); // llamamos una funcion auxiliar (esta en auxiliar.cpp)
	
	if (!map) // comprobamos que se ha generado bien la textura, si no saca un codigo de error
	{
		std::cout << "Error cargando el fichero: "
			<< fileName << std::endl;
		exit(-1);
	}

	unsigned int texId;
	glGenTextures(1, &texId); //crea la textura
	glBindTexture(GL_TEXTURE_2D, texId); // vincula la textura al buffer y nos asiogna un id nuevo
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, //subimos la textura a la tarjeta grafica
		GL_UNSIGNED_BYTE, (GLvoid*)map); 
	glGenerateMipmap(GL_TEXTURE_2D); //generamos mipmap de la textura


	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);

	delete[] map; //libera memoria de la CPU

	return texId; //devolvemos el id de la textura q acabamos de crear
}

void renderFunc(){
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //limpiar buffer de color y buffer de profundidad
	// Limpias el renderizado anterior antes que nada 

	// Usamos el programa que hemos definidom, podremos usar otros programas
	glUseProgram(program);

	// Calculamos las matrices requeridas por el shader de vertices
	glm::mat4 modelView = view * model;
	glm::mat4 modelViewProj = proj * view * model;
	glm::mat4 normal = glm::transpose(glm::inverse(modelView));

	//Con los identificadores de las matrices, subimos dichas matrices
	if (uModelViewMat != -1) // comprobamos que esa matriz está, (su id será -1 si el shader no al encuentra, es el valor por defecto)
		glUniformMatrix4fv(uModelViewMat, 1, GL_FALSE, //si está, le otorga un valor (?????)
			&(modelView[0][0]));
	if (uModelViewProjMat != -1)
		glUniformMatrix4fv(uModelViewProjMat, 1, GL_FALSE,
			&(modelViewProj[0][0]));
	if (uNormalMat != -1)
		glUniformMatrix4fv(uNormalMat, 1, GL_FALSE,
			&(normal[0][0]));

	//Texturas
	if (uColorTex != -1) //comprobamos que el shader ha pillado la textura, uColorTex es el id en el shader
	{
		glActiveTexture(GL_TEXTURE0); //activamos la textura con el id
		glBindTexture(GL_TEXTURE_2D, colorTexId); //vinculamos la textura 
		glUniform1i(uColorTex, 0); //asignamos al uniform el identificador de la textura en el shader
	}
	if (uEmiTex != -1)
	{
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, emiTexId);
		glUniform1i(uEmiTex, 1);
	}

	glBindVertexArray(vao); //enlazamos con el vao
	glDrawElements(GL_TRIANGLES, cubeNTriangleIndex * 3, //dibujamos los poligonos
		GL_UNSIGNED_INT, (void*)0);

	glutSwapBuffers(); // cuando estamos pintando pintamos en el default frame buffer (lienzo) 
	// Hay que pasarlo a la pantalla, es decir, hay que hacer un swap
}

void resizeFunc(int width, int height){
	glViewport(0, 0, width, height); //ajustamos el viewport con la anchura y altura dadas
	glutPostRedisplay();
}

void idleFunc(){
	model = glm::mat4(1.0f);
	static float angle = 0.0f; // se va actualizando este valor estatico
	angle = (angle > 3.141592f * 2.0f) ? 0 : angle + 0.01f;

	model = glm::rotate(model, angle, glm::vec3(1.0f, 1.0f, 0.0f));
	glutPostRedisplay();
}
void keyboardFunc(unsigned char key, int x, int y){}
void mouseFunc(int button, int state, int x, int y){}









