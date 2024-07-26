//internal includes
#include "common.h"
#include "ShaderProgram.h"
#include "Camera.h"

//External dependencies
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>
#include <random>

static const GLsizei WIDTH = 1280, HEIGHT = 768; //размеры окна
static int filling = 0;
static bool normals_mode = 0;
static bool texture_mode = 0;
static bool fog_mode     = 0;

static bool keys[1024]; // массив состояний кнопок - 
						// нажата/не нажата
static GLfloat lastX = 400, lastY = 300; // исходное положение мыши
static bool firstMouse = true;
static bool g_captureMouse = true;	// Мышка захвачена 
									// нашим приложением или нет?
static bool g_capturedMouseJustNow = false;

static const float roughness = 0.035f;
static const float water_level = 1.1f;
static const float plane_size  = 40.0f;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

Camera camera(float3(0.0f, 5.0f, 30.0f));

//функция для обработки нажатий на кнопки клавиатуры
void OnKeyboardPressed(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	//std::cout << key << std::endl;
	switch (key)
	{

	case GLFW_KEY_ESCAPE: //на Esc выходим из программы
		if (action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GL_TRUE);
		break;

	case GLFW_KEY_SPACE: 
		//на пробел переключение в каркасный режим и обратно
		if (action == GLFW_PRESS)
		{
			if (filling == 0)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				filling = 1;
			}
			else
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				filling = 0;
			}
		}
		break;

	case GLFW_KEY_1:

		normals_mode = 0;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		break;

	case GLFW_KEY_2:

		normals_mode = 1;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		break;

	case GLFW_KEY_T:
		
		if (action == GLFW_PRESS)
			texture_mode = (texture_mode)? 0: 1;
		break;

	case GLFW_KEY_F:
	
		if (action == GLFW_PRESS)
			fog_mode = (fog_mode)? 0: 1;
		break;

	default:
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
			keys[key] = false;
	}
}

//функция для обработки клавиш мыши
void OnMouseButtonClicked(GLFWwindow* window, int button, 
						  int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && 
		action == GLFW_RELEASE)

		g_captureMouse = !g_captureMouse;

	if (g_captureMouse)
	{
		glfwSetInputMode(window, GLFW_CURSOR, 
						 GLFW_CURSOR_DISABLED);
		g_capturedMouseJustNow = true;
	}

	else
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

}

//функция для обработки перемещения мыши
void OnMouseMove(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = float(xpos);
		lastY = float(ypos);
		firstMouse = false;
	}

	GLfloat xoffset = float(xpos) - lastX;
	GLfloat yoffset = lastY - float(ypos);	

	lastX = float(xpos);
	lastY = float(ypos);

	if (g_captureMouse)
		camera.ProcessMouseMove(xoffset, yoffset);
}


void OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(GLfloat(yoffset));
}

void doCameraMovement(Camera &camera, GLfloat deltaTime)
{
	if (keys[GLFW_KEY_W])
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_A])
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_S])
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_D])
		camera.ProcessKeyboard(RIGHT, deltaTime);
}


float mean4(const float &a, const float &b, const float &c,
			const float &d = 0)
{
	return (a+b+c+d) / 4;
}


void doSquareStep(int side, int map_size, std::vector<std::vector<float> > &heights)
{
	//std::cout << "SQUARE side = " << side << std::endl;

	for (int z = 0; z < map_size-1; z += side)
		for (int x = 0; x < map_size-1; x += side)

	heights[z + side/2][x + side/2] = 
			mean4(heights[z][x], 
				  heights[z+side][x], 
				  heights[z][x+side],
				  heights[z+side][x+side]) + 
			rnd(-side*roughness, side*roughness);
}

void doDiamondStep(int side, int map_size, std::vector<std::vector<float> > &heights)
{
	//std::cout << "DIAMOND side = " << side << std::endl;

	for (int z = 0; z < map_size-1; z += side/2)
		for (int x = (z+side/2)%side; x < map_size-1; x += side)
	{
		heights[z][x] = 
			mean4(heights[(z-side/2 + map_size-1)%(map_size-1)][x], 
				  heights[(z+side/2)%(map_size-1)][x], 
				  heights[z][(x+side/2)%(map_size-1)], 
				  heights[z][(x-side/2 + map_size-1)%(map_size-1)]) +
			rnd(-side*roughness, side*roughness);

		if (z == 0)
			heights[map_size-1][x] = heights[z][x];

		if (x == 0)
			heights[z][map_size-1] = heights[z][x];
	}
}


// assume that map_size is a power of 2 + 1
void generateLandscape(int map_size, std::vector<std::vector<float> > &heights)
{
	int side;

	// initial values on corners
	srand(time(NULL));

	heights[0][0] = rnd(3.0f, 5.0f);
	heights[0][map_size-1] = rnd(3.0f, 5.0f);
	heights[map_size-1][0] = rnd(3.0f, 5.0f);
	heights[map_size-1][map_size-1] = rnd(3.0f, 5.0f);

	for (side = map_size-1; side > 1; side /= 2)
	{
		//std::cout << "MAIN side = " << side << std::endl;

		doSquareStep(side, map_size, heights);
		doDiamondStep(side, map_size, heights);
	}
}

void normalizeLandscape(int map_size, std::vector<std::vector<float> > &heights)
{
	float min_level, max_level;

	min_level = max_level = heights[0][0];

	for (int z = 0; z < map_size-1; z++)
		for (int x = 0; x < map_size-1; x++)
	{
		float element = heights[z][x];

		if (element > max_level)
			max_level = element;

		if (element < min_level)
			min_level = element;
	}

	// new_y = (y - min)/(max - min)

	for (int z = 0; z < map_size; z++)
		for (int x = 0; x < map_size; x++)

		heights[z][x] = (((heights[z][x] - min_level) / 
						(max_level - min_level)) * 
						((heights[z][x] - min_level) / 
						(max_level - min_level))) * 5.0f;
}



/**
\brief создать triangle strip плоскость и загрузить её в шейдерную программу
\param rows - число строк
\param cols - число столбцов
\param size - размер плоскости
\param vao - vertex array object, связанный с созданной плоскостью
*/
static int createTriStrip(int rows, int cols, float size, 
						  GLuint &vao, bool is_plane)
{
	int numIndices = 2 * cols*(rows - 1) + rows - 1;

	//вектор атрибута координат вершин
	std::vector<GLfloat> vertices_vec;
	vertices_vec.reserve(rows * cols * 3);

	//вектор атрибута нормалей к вершинам
	std::vector<GLfloat> normals_vec; 
	normals_vec.reserve(rows * cols * 3);

	//вектор атрибут текстурных координат вершин
	std::vector<GLfloat> texcoords_vec; 
	texcoords_vec.reserve(rows * cols * 2);

	//временный вектор нормалей, используемый для расчетов
	std::vector<float3> normals_vec_tmp(rows * cols, 
										float3(0.0f, 0.0f, 0.0f));

	// вектор граней (треугольников), каждая грань - 
	// три индекса вершин, её составляющих; 
	// используется для удобства расчета нормалей
	std::vector<int3> faces; 
	faces.reserve(numIndices / 3);

	//вектор индексов вершин для передачи шейдерной программе
	std::vector<GLuint> indices_vec; 	
	indices_vec.reserve(numIndices);

	int height_size = max(rows, cols);
	std::vector<std::vector<float> > heights(height_size, std::vector<float>(height_size));

	if (!is_plane)
	{
		float min_level, max_level;

		generateLandscape(height_size, heights);
		normalizeLandscape(height_size, heights);
	}

	for (int z = 0; z < rows; ++z)
	{
		for (int x = 0; x < cols; ++x)
		{
			//вычисляем координаты каждой из вершин 
			float xx = -size / 2 + x*size / cols;
			float zz = -size / 2 + z*size / rows;
			float r = sqrt(xx*xx + zz*zz);
			float yy;
			
			if (is_plane)
				yy = water_level;

			else
				yy = heights[z][x];

			vertices_vec.push_back(xx);
			vertices_vec.push_back(yy);
			vertices_vec.push_back(zz);

			// вычисляем первую текстурную координату u, 
			// для плоскости это просто относительное 
			// положение вершины
			texcoords_vec.push_back(x / float(cols - 1));
			// аналогично вычисляем вторую текстурную координату v
			texcoords_vec.push_back(z / float(rows - 1));
		}
	}

	//primitive restart - специальный индекс, который 
	// обозначает конец строки из треугольников в triangle_strip

	// после этого индекса формирование треугольников из 
	// массива индексов начнется заново - будут взяты 
	// следующие 3 индекса для первого треугольника
	// и далее каждый последующий индекс будет добавлять 
	// один новый треугольник пока снова не 
	// встретится primitive restart index

	int primRestart = cols * rows;

	for (int x = 0; x < cols - 1; ++x)
	{
		for (int z = 0; z < rows - 1; ++z)
		{
			int offset = x*cols + z;

			// каждую итерацию добавляем по два треугольника, 
			// которые вместе формируют четырехугольник
			if (z == 0) 
			// если мы в начале строки треугольников, 
			// нам нужны первые четыре индекса
			{
				indices_vec.push_back(offset + 0);
				indices_vec.push_back(offset + rows);
				indices_vec.push_back(offset + 1);
				indices_vec.push_back(offset + rows + 1);
			}

			else 
			// иначе нам достаточно двух индексов, 
			// чтобы добавить два треугольника
			{
				indices_vec.push_back(offset + 1);
				indices_vec.push_back(offset + rows + 1);

				// если мы дошли до конца строки, 
				// вставляем primRestart, 
				// чтобы обозначить переход на следующую строку
				if (z == rows - 2) 
					indices_vec.push_back(primRestart);
			}
		}
	}

	///////////////////////
	//формируем вектор граней(треугольников) по 3 индекса на каждый
	int currFace = 1;

	for (int i = 0; i < indices_vec.size() - 2; ++i)
	{
		int3 face;

		int index0 = indices_vec.at(i);
		int index1 = indices_vec.at(i + 1);
		int index2 = indices_vec.at(i + 2);

		if (index0 != primRestart && 
			index1 != primRestart && 
			index2 != primRestart)
		{
			// если это нечетный треугольник, то индексы 
			// и так в правильном порядке обхода - 
			// против часовой стрелки
			if (currFace % 2 != 0)
			{
				face.x = indices_vec.at(i);
				face.y = indices_vec.at(i + 1);
				face.z = indices_vec.at(i + 2);

				currFace++;
			}

			// если треугольник четный, то нужно поменять 
			// местами 2-й и 3-й индекс;
			else
			{		
				// при отрисовке opengl делает это за нас, но 
				// при расчете нормалей нам нужно это 
				// сделать самостоятельно
				face.x = indices_vec.at(i);
				face.y = indices_vec.at(i + 2);
				face.z = indices_vec.at(i + 1);

				currFace++;
			}
			faces.push_back(face);
		}
	}


	///////////////////////
	//расчет нормалей
	for (int i = 0; i < faces.size(); ++i)
	{
		// получаем из вектора вершин координаты каждой из 
		// вершин одного треугольника
		float3 A(vertices_vec.at(3 * faces.at(i).x + 0), 
				 vertices_vec.at(3 * faces.at(i).x + 1), 
				 vertices_vec.at(3 * faces.at(i).x + 2));

		float3 B(vertices_vec.at(3 * faces.at(i).y + 0), 
				 vertices_vec.at(3 * faces.at(i).y + 1), 
				 vertices_vec.at(3 * faces.at(i).y + 2));

		float3 C(vertices_vec.at(3 * faces.at(i).z + 0), 
				 vertices_vec.at(3 * faces.at(i).z + 1), 
				 vertices_vec.at(3 * faces.at(i).z + 2));

		//получаем векторы для ребер треугольника из каждой из 3-х вершин
		float3 edge1A(normalize(B - A));
		float3 edge2A(normalize(C - A));

		float3 edge1B(normalize(A - B));
		float3 edge2B(normalize(C - B));

		float3 edge1C(normalize(A - C));
		float3 edge2C(normalize(B - C));

		//нормаль к треугольнику - векторное произведение любой пары векторов из одной вершины
		float3 face_normal = cross(edge1A, edge2A);

		//простой подход: нормаль к вершине = средняя по треугольникам, к которым принадлежит вершина
		normals_vec_tmp.at(faces.at(i).x) += face_normal;
		normals_vec_tmp.at(faces.at(i).y) += face_normal;
		normals_vec_tmp.at(faces.at(i).z) += face_normal;
	}

	// нормализуем векторы нормалей и записываем их в вектор 
	// из GLFloat, который будет передан в шейдерную программу
	for (int i = 0; i < normals_vec_tmp.size(); ++i)
	{
		float3 N = normalize(normals_vec_tmp.at(i));

		normals_vec.push_back(N.x);
		normals_vec.push_back(N.y);
		normals_vec.push_back(N.z);
	}


	GLuint vboVertices, vboIndices, vboNormals, vboTexCoords;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vboVertices);
	glGenBuffers(1, &vboIndices);
	glGenBuffers(1, &vboNormals);
	glGenBuffers(1, &vboTexCoords);


	glBindVertexArray(vao); 
	GL_CHECK_ERRORS;

	{

		//передаем в шейдерную программу атрибут координат вершин
		glBindBuffer(GL_ARRAY_BUFFER, vboVertices); 
		GL_CHECK_ERRORS;
		glBufferData(GL_ARRAY_BUFFER, 
					 vertices_vec.size() * sizeof(GL_FLOAT), 
					 &vertices_vec[0], GL_STATIC_DRAW); 
		GL_CHECK_ERRORS;
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
							  3 * sizeof(GL_FLOAT), 
							  (GLvoid *) 0); 
		GL_CHECK_ERRORS;
		glEnableVertexAttribArray(0); 
		GL_CHECK_ERRORS;

		//передаем в шейдерную программу атрибут нормалей
		glBindBuffer(GL_ARRAY_BUFFER, vboNormals); 
		GL_CHECK_ERRORS;
		glBufferData(GL_ARRAY_BUFFER, 
					 normals_vec.size() * sizeof(GL_FLOAT), 
					 &normals_vec[0], GL_STATIC_DRAW); 
		GL_CHECK_ERRORS;
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 
							  3 * sizeof(GL_FLOAT), 
							  (GLvoid *) 0); 
		GL_CHECK_ERRORS;
		glEnableVertexAttribArray(1); 
		GL_CHECK_ERRORS;

		// передаем в шейдерную программу атрибут 
		// текстурных координат
		glBindBuffer(GL_ARRAY_BUFFER, vboTexCoords); 
		GL_CHECK_ERRORS;
		glBufferData(GL_ARRAY_BUFFER, 
					 texcoords_vec.size() * sizeof(GL_FLOAT), 
					 &texcoords_vec[0], 
					 GL_STATIC_DRAW); 
		GL_CHECK_ERRORS;
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 
							  2 * sizeof(GL_FLOAT), 
							  (GLvoid*)0); 
		GL_CHECK_ERRORS;
		glEnableVertexAttribArray(2); 
		GL_CHECK_ERRORS;

		//передаем в шейдерную программу индексы
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices); 
		GL_CHECK_ERRORS;
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
					 indices_vec.size() * sizeof(GLuint), 
					 &indices_vec[0], 
					 GL_STATIC_DRAW); 
		GL_CHECK_ERRORS;

		glEnable(GL_PRIMITIVE_RESTART); 
		GL_CHECK_ERRORS;
		glPrimitiveRestartIndex(primRestart); 
		GL_CHECK_ERRORS;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return numIndices;
}


int createSkyBox(GLuint &vao, float size)
{
	std::vector<GLfloat> vertices;

	//front
	vertices.push_back(-size);
	vertices.push_back(-size);
	vertices.push_back(size);

	vertices.push_back(size); 
	vertices.push_back(-size); 
	vertices.push_back(size);

	vertices.push_back(size); 
	vertices.push_back(size); 
	vertices.push_back(size);

	vertices.push_back(-size); 
	vertices.push_back(size); 
	vertices.push_back(size);
 
	//right
	vertices.push_back(size);
	vertices.push_back(size);
	vertices.push_back(size);

	vertices.push_back(size); 
	vertices.push_back(size); 
	vertices.push_back(-size);

	vertices.push_back(size); 
	vertices.push_back(-size); 
	vertices.push_back(-size);

	vertices.push_back(size); 
	vertices.push_back(-size); 
	vertices.push_back(size);
 
	//back
	vertices.push_back(-size);
	vertices.push_back(-size);
	vertices.push_back(-size);

	vertices.push_back(size); 
	vertices.push_back(-size); 
	vertices.push_back(-size);

	vertices.push_back(size); 
	vertices.push_back(size); 
	vertices.push_back(-size);

	vertices.push_back(-size); 
	vertices.push_back(size); 
	vertices.push_back(-size);
 
	//left
	vertices.push_back(-size);
	vertices.push_back(-size);
	vertices.push_back(-size);

	vertices.push_back(-size); 
	vertices.push_back(-size); 
	vertices.push_back(size);

	vertices.push_back(-size); 
	vertices.push_back(size); 
	vertices.push_back(size);

	vertices.push_back(-size); 
	vertices.push_back(size); 
	vertices.push_back(-size);
 
	//upper
	vertices.push_back(size);
	vertices.push_back(size);
	vertices.push_back(size);

	vertices.push_back(-size); 
	vertices.push_back(size); 
	vertices.push_back(size);

	vertices.push_back(-size); 
	vertices.push_back(size); 
	vertices.push_back(-size);

	vertices.push_back(size); 
	vertices.push_back(size); 
	vertices.push_back(-size);

 
	//bottom
	vertices.push_back(-size);
	vertices.push_back(-size);
	vertices.push_back(-size);

	vertices.push_back(size); 
	vertices.push_back(-size); 
	vertices.push_back(-size);

	vertices.push_back(size); 
	vertices.push_back(-size); 
	vertices.push_back(size);

	vertices.push_back(-size); 
	vertices.push_back(-size); 
	vertices.push_back(size);

	std::vector<GLuint> 
		indices = { 0, 1, 2, 0, 2, 3, //front
					4, 5, 6, 4, 6, 7, //right
					8, 9, 10, 8, 10, 11, //back
					12, 13, 14, 12, 14, 15, //left
					16, 17, 18, 16, 18, 19, //upper
					20, 21, 22, 20, 22, 23};

	GLuint vbo_vertices, vbo_indices;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo_vertices);
	glGenBuffers(1, &vbo_indices);

	glBindVertexArray(vao);
	GL_CHECK_ERRORS;

	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), 
					 &vertices[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
							  3 * sizeof(GL_FLOAT), (GLvoid *) 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
					 indices.size() * sizeof(GLuint), 
					 &indices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	glBindVertexArray(0);
	return indices.size();
}


int initGL()
{
	int res = 0;

	//грузим функции opengl через glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << 
			std::endl;

		return -1;
	}

	//выводим в консоль некоторую информацию о драйвере и контексте opengl
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
	std::cout << "===============================" << std::endl;

	std::cout << "Controls: "<< std::endl;
	std::cout << "press LBM to capture/release mouse cursor	"<< std::endl;
	std::cout << "press spacebar to alternate between shaded wireframe and fill display modes" << std::endl;
	std::cout << "press t to switch to texture visualization" << std::endl;
	std::cout << "press f to enable fog" << std::endl;
	std::cout << "press 2 to activate normals view" << std::endl;
	std::cout << "press ESC to exit" << std::endl;

	return 0;
}

int main(int argc, char** argv)
{
	if (!glfwInit())
		return -1;

	//запрашиваем контекст opengl версии 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); 
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); 

	GLFWwindow*	window = 
		glfwCreateWindow(WIDTH, HEIGHT, "OpenGL basic sample", 
						 nullptr, nullptr);

	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	
	glfwMakeContextCurrent(window); 

	// регистрируем коллбеки для обработки сообщений 
	// от пользователя - клавиатура, мышь.
	glfwSetKeyCallback(window, OnKeyboardPressed);	
	glfwSetCursorPosCallback(window, OnMouseMove); 
	glfwSetMouseButtonCallback(window, OnMouseButtonClicked);
	glfwSetScrollCallback(window, OnMouseScroll);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 

	if (initGL() != 0) 
		return -1;
	
	//Reset any OpenGL errors which could be present for some reason
	GLenum gl_error = glGetError();

	while (gl_error != GL_NO_ERROR)
		gl_error = glGetError();

	//создание шейдерной программы из двух файлов с исходниками шейдеров
	//используется класс-обертка ShaderProgram
	std::unordered_map<GLenum, std::string> shaders_landscape, 
											shaders_water, 
											shaders_skybox;

	shaders_landscape[GL_VERTEX_SHADER] = "vertexLandscape.glsl";
	shaders_landscape[GL_FRAGMENT_SHADER] = "fragmentLandscape.glsl";
	ShaderProgram program_landscape(shaders_landscape); 

	shaders_water[GL_VERTEX_SHADER] = "vertexWater.glsl";
	shaders_water[GL_FRAGMENT_SHADER] = "fragmentWater.glsl";
	ShaderProgram program_water(shaders_water);

	shaders_skybox[GL_VERTEX_SHADER] = "vertexSkybox.glsl";
	shaders_skybox[GL_FRAGMENT_SHADER] = "fragmentSkybox.glsl";
	ShaderProgram program_skybox(shaders_skybox);

	GL_CHECK_ERRORS;

	
	//Создаем и загружаем геометрию поверхности
	GLuint vaoTriStrip;
	int triStripIndices = createTriStrip(257, 257, plane_size, vaoTriStrip, 0);

	//Создаем и загружаем поверхность воды
	GLuint vao_water;
	int water_indices = createTriStrip(257, 257, plane_size, vao_water, 1);

	GLuint vao_skybox;
	int skybox_indices = createSkyBox(vao_skybox, 3*plane_size);

	// Texture loading

	GLuint texture_snow, texture_rock, texture_grass, 
		   texture_sand, texture_water, 
		   texture_skybox;

	unsigned char *image_snow, *image_rock, *image_grass, 
				  *image_sand, *image_water, 
				  
				  *sky_front, *sky_back, *sky_top, 
				  *sky_bottom, *sky_left, *sky_right;
	int tw, th;

	glGenTextures(1, &texture_snow);
	glGenTextures(1, &texture_rock);
	glGenTextures(1, &texture_grass);
	glGenTextures(1, &texture_sand);
	glGenTextures(1, &texture_water);

	glGenTextures(1, &texture_skybox);

	glBindTexture(GL_TEXTURE_2D, texture_snow);
	image_snow = SOIL_load_image("../textures/snow.jpg", 
								  &tw, &th, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB, 
				 GL_UNSIGNED_BYTE, image_snow);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image_snow);
	GL_CHECK_ERRORS;

	glBindTexture(GL_TEXTURE_2D, texture_rock);
	image_rock = SOIL_load_image("../textures/rock.jpg", 
								  &tw, &th, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB, 
				 GL_UNSIGNED_BYTE, image_rock);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image_rock);
	GL_CHECK_ERRORS;

	glBindTexture(GL_TEXTURE_2D, texture_grass);
	image_grass = SOIL_load_image("../textures/grass.jpg", 
								  &tw, &th, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB, 
				 GL_UNSIGNED_BYTE, image_grass);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image_grass);

	glBindTexture(GL_TEXTURE_2D, texture_sand);
	image_sand = SOIL_load_image("../textures/sand.jpg", 
								  &tw, &th, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB, 
				 GL_UNSIGNED_BYTE, image_sand);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image_sand);

	glBindTexture(GL_TEXTURE_2D, texture_water);
	image_water = SOIL_load_image("../textures/water.jpg", 
								  &tw, &th, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB, 
				 GL_UNSIGNED_BYTE, image_water);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image_water);


	// we have the same sizes for all images of skybox
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture_skybox);
	
	sky_front = SOIL_load_image("../textures/skybox/negz.jpg", 
								&tw, &th, 0, SOIL_LOAD_RGB);
	sky_back = SOIL_load_image("../textures/skybox/posz.jpg", 
								&tw, &th, 0, SOIL_LOAD_RGB);
	sky_top = SOIL_load_image("../textures/skybox/posy.jpg", 
								&tw, &th, 0, SOIL_LOAD_RGB);
	sky_bottom = SOIL_load_image("../textures/skybox/negy.jpg", 
								&tw, &th, 0, SOIL_LOAD_RGB);
	sky_left = SOIL_load_image("../textures/skybox/negx.jpg", 
								&tw, &th, 0, SOIL_LOAD_RGB);
	sky_right = SOIL_load_image("../textures/skybox/posx.jpg", 
								&tw, &th, 0, SOIL_LOAD_RGB);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, tw, th, 0, GL_RGB, GL_UNSIGNED_BYTE, sky_front);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, tw, th, 0, GL_RGB, GL_UNSIGNED_BYTE, sky_back);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, tw, th, 0, GL_RGB, GL_UNSIGNED_BYTE, sky_top);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, tw, th, 0, GL_RGB, GL_UNSIGNED_BYTE, sky_bottom);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, tw, th, 0, GL_RGB, GL_UNSIGNED_BYTE, sky_left);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, tw, th, 0, GL_RGB, GL_UNSIGNED_BYTE, sky_right);

	GL_CHECK_ERRORS;
	// FIXME: this for some reason was failing while I was trying
	// to launch the project a few years after the development.
	// Try disabling this function call to be able to run the program.
	// In this case, the skybox will be black.
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	GL_CHECK_ERRORS;

	SOIL_free_image_data(sky_front);
	SOIL_free_image_data(sky_back);
	SOIL_free_image_data(sky_top);
	SOIL_free_image_data(sky_bottom);
	SOIL_free_image_data(sky_left);
	SOIL_free_image_data(sky_right);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
 
	//// end texture loading

	glViewport(0, 0, WIDTH, HEIGHT);
	GL_CHECK_ERRORS;
	glEnable(GL_DEPTH_TEST);
	GL_CHECK_ERRORS;
	glEnable(GL_BLEND);
	GL_CHECK_ERRORS;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//цикл обработки сообщений и отрисовки сцены каждый кадр
	while (!glfwWindowShouldClose(window))
	{
		//считаем сколько времени прошло за кадр
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		doCameraMovement(camera, deltaTime);

		//очищаем экран каждый кадр
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		GL_CHECK_ERRORS;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
		GL_CHECK_ERRORS;

		GLint light_location = glGetUniformLocation(program_landscape.GetProgram(), "light_pos");
		GLfloat light_x  = 1.0f * cos(0.1f * currentFrame);
		GLfloat light_y  = 1.0f * sin(0.1f * currentFrame);
		////

		program_landscape.StartUseShader(); 
		GL_CHECK_ERRORS;

		glUniform3f(light_location, light_x, light_y, 0.0f);
		////

		//обновляем матрицы камеры и проекции каждый кадр
		float4x4 view = camera.GetViewMatrix();
		float4x4 projection = 
			projectionMatrixTransposed(camera.zoom, 
									   float(WIDTH) / 
									   float(HEIGHT), 0.1f, 
									   1000.0f);

		// модельная матрица, определяющая положение объекта 
		// в мировом пространстве
		float4x4 model; //начинаем с единичной матрицы
		
		//загружаем uniform-переменные в шейдерную программу 
		// (одинаковые для всех параллельно запускаемых 
		// копий шейдера)

		// Textures //

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_snow);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture_rock);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, texture_grass);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, texture_sand);

		program_landscape.SetUniform("texture_snow", 0);
		GL_CHECK_ERRORS;
		program_landscape.SetUniform("texture_rock", 1);
		GL_CHECK_ERRORS;
		program_landscape.SetUniform("texture_grass", 2);
		GL_CHECK_ERRORS;
		program_landscape.SetUniform("texture_sand", 3);
		GL_CHECK_ERRORS;

		program_landscape.SetUniform("texture_mode", texture_mode);
		GL_CHECK_ERRORS;

		////

		program_landscape.SetUniform("view", view);			 
		GL_CHECK_ERRORS;
		program_landscape.SetUniform("projection", projection); 
		GL_CHECK_ERRORS;
		program_landscape.SetUniform("model", model);
		GL_CHECK_ERRORS;
		program_landscape.SetUniform("normals_mode", normals_mode);
		GL_CHECK_ERRORS;
		program_landscape.SetUniform("fog_mode", fog_mode);
		GL_CHECK_ERRORS;

		//рисуем плоскость
		glBindVertexArray(vaoTriStrip);
		glDrawElements(GL_TRIANGLE_STRIP, triStripIndices, 
					   GL_UNSIGNED_INT, nullptr); 
		GL_CHECK_ERRORS;
		glBindVertexArray(0); 
		GL_CHECK_ERRORS;

		program_landscape.StopUseShader();

		// ====== WATER ====== //

		program_water.StartUseShader();

		light_location = glGetUniformLocation(program_water.GetProgram(), "light_pos");
		glUniform3f(light_location, light_x, light_y, 0.0f);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, texture_water);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture_skybox);

		program_water.SetUniform("texture_water", 4);
		GL_CHECK_ERRORS;
		program_water.SetUniform("texture_skybox", 5);
		GL_CHECK_ERRORS;

		program_water.SetUniform("texture_mode", texture_mode);
		GL_CHECK_ERRORS;
		program_water.SetUniform("fog_mode", fog_mode);
		GL_CHECK_ERRORS;
		program_water.SetUniform("view", view);	
		GL_CHECK_ERRORS;
		program_water.SetUniform("projection", projection); 
		GL_CHECK_ERRORS;
		program_water.SetUniform("model", model);
		GL_CHECK_ERRORS;
		program_water.SetUniform("time", currentFrame);
		GL_CHECK_ERRORS;

		glBindVertexArray(vao_water);
		glDrawElements(GL_TRIANGLE_STRIP, water_indices, 
					   GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		program_water.StopUseShader();

		// ===== SKYBOX ===== //

		program_skybox.StartUseShader();

		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture_skybox);

		program_skybox.SetUniform("sky_box", 6);
		GL_CHECK_ERRORS;
		program_skybox.SetUniform("view", view);	
		GL_CHECK_ERRORS;
		program_skybox.SetUniform("projection", projection); 
		GL_CHECK_ERRORS;
		program_skybox.SetUniform("model", model);
		GL_CHECK_ERRORS;
		program_skybox.SetUniform("camera_position", camera.pos);
		GL_CHECK_ERRORS;

		glBindVertexArray(vao_skybox);

		//int size;
		//glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, 
		//					   GL_BUFFER_SIZE, &size);
		//glDrawElements(GL_TRIANGLES, size/sizeof(GLuint), 

		glDrawElements(GL_TRIANGLES, skybox_indices, 
					   GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		program_skybox.StopUseShader();
		////

		glfwSwapBuffers(window); 
	}

	//очищаем vao перед закрытием программы
	glDeleteVertexArrays(1, &vaoTriStrip);
	glDeleteVertexArrays(1, &vao_water);

	glfwTerminate();
	return 0;
}
