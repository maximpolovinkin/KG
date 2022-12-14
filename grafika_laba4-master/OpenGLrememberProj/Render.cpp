#include "Render.h"

#include <windows.h>

#include <GL\gl.h>
#include <GL\glu.h>
#include "glext.h"

#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "MyShaders.h"

#include "ObjLoader.h"
#include "GUItextRectangle.h"

#include "Texture.h"

#include <string.h>


GuiTextRectangle rec;

bool textureMode = true;
bool lightMode = true;
bool idioticMode = false;


//небольшой дефайн для упрощения кода
#define POP glPopMatrix()
#define PUSH glPushMatrix()


ObjFile *model;

Texture texture1;
Texture sTex;
Texture rTex;
Texture tBox;

Shader s[10];  //массивчик для десяти шейдеров
Shader frac;
Shader cassini;



double manPosition[] = { -10,-10,0 };

//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;

	
	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}

	
	//считает позицию камеры, исходя из углов поворота, вызывается движком
	virtual void SetUpCamera()
	{

		lookPoint.setCoords(-manPosition[1], manPosition[0], manPosition[2]);

		pos.setCoords(camDist*cos(fi2)*cos(fi1)-manPosition[1],
			camDist*cos(fi2)*sin(fi1)+manPosition[0],
			camDist*sin(fi2)+manPosition[2]);

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
		
	}



}  camera;   //создаем объект камеры


//класс недоделан!
class WASDcamera :public CustomCamera
{
public:
		
	float camSpeed;

	WASDcamera()
	{
		camSpeed = 0.4;
		pos.setCoords(5, 5, 5);
		lookPoint.setCoords(0, 0, 0);
		normal.setCoords(0, 0, 1);
	}

	virtual void SetUpCamera()
	{

		if (OpenGL::isKeyPressed('W'))
		{
			Vector3 forward = (lookPoint - pos).normolize()*camSpeed;
			pos = pos + forward;
			lookPoint = lookPoint + forward;
			
		}
		if (OpenGL::isKeyPressed('S'))
		{
			Vector3 forward = (lookPoint - pos).normolize()*(-camSpeed);
			pos = pos + forward;
			lookPoint = lookPoint + forward;
			
		}

		LookAt();
	}

} WASDcam;


//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}

	
	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		Shader::DontUseShaders();
		bool f1 = glIsEnabled(GL_LIGHTING);
		glDisable(GL_LIGHTING);
		bool f2 = glIsEnabled(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_2D);
		bool f3 = glIsEnabled(GL_DEPTH_TEST);
		
		glDisable(GL_DEPTH_TEST);
		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale*0.08;
		s.Show();

		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
				glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale*1.5;
			c.Show();
		}
		/*
		if (f1)
			glEnable(GL_LIGHTING);
		if (f2)
			glEnable(GL_TEXTURE_2D);
		if (f3)
			glEnable(GL_DEPTH_TEST);
			*/
	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света



//старые координаты мыши
int mouseX = 0, mouseY = 0;




float offsetX = 0, offsetY = 0;
float zoom=1;
float Time = 0;
int tick_o = 0;
int tick_n = 0;

//обработчик движения мыши
void mouseEvent(OpenGL *ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01*dx;
		camera.fi2 += -0.01*dy;
	}


	if (OpenGL::isKeyPressed(VK_LBUTTON))
	{
		offsetX -= 1.0*dx/ogl->getWidth()/zoom;
		offsetY += 1.0*dy/ogl->getHeight()/zoom;
	}


	
	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y,60,ogl->aspect);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k*r.direction.X() + r.origin.X();
		y = k*r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02*dy);
	}

	
}

//обработчик вращения колеса  мыши
void mouseWheelEvent(OpenGL *ogl, int delta)
{


	float _tmpZ = delta*0.003;
	if (ogl->isKeyPressed('Z'))
		_tmpZ *= 10;
	zoom += 0.2*zoom*_tmpZ;


	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01*delta;
}


//обработчик нажатия кнопок клавиатуры
void keyDownEvent(OpenGL *ogl, int key)
{
	if (key == 'L')
	{
		lightMode = !lightMode;
	}

	if (key == 'T')
	{
		textureMode = !textureMode;
	}	   

	if (key == 'R')
	{
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
	}

	if (key == 'F')
	{
		light.pos = camera.pos;
	}

	if (OpenGL::isKeyPressed('R'))
	{
		frac.LoadShaderFromFile();
		frac.Compile();

		s[0].LoadShaderFromFile();
		s[0].Compile();

		cassini.LoadShaderFromFile();
		cassini.Compile();
	}

	if (key == 'Q')
		Time = 0;

	if (OpenGL::isKeyPressed('W')) {
		manPosition[1] += 1;
		
	}

	if (OpenGL::isKeyPressed('A')) {
		manPosition[0] -= 0.2;
	}

	if (OpenGL::isKeyPressed('D'))
	{
		manPosition[0] += 0.2;
	}

	if (OpenGL::isKeyPressed('S'))
	{
		manPosition[1] -= 0.2;
	}

	if (OpenGL::isKeyPressed('M'))
	{
		idioticMode = !idioticMode;
	}

}



void keyUpEvent(OpenGL *ogl, int key)
{
	
}


void DrawQuad()
{
	double A[] = { 0,0 };
	double B[] = { 1,0 };
	double C[] = { 1,1 };
	double D[] = { 0,1 };
	glBegin(GL_QUADS);
	glColor3d(.5, 0, 0);
	glNormal3d(0, 0, 1);
	glTexCoord2d(0, 0);
	glVertex2dv(A);
	glTexCoord2d(1, 0);
	glVertex2dv(B);
	glTexCoord2d(1, 1);
	glVertex2dv(C);
	glTexCoord2d(0, 1);
	glVertex2dv(D);
	glEnd();
}


char* con(const char* first, const char* second) {
	int l1 = 0, l2 = 0;
	const char* f = first, * l = second;

	// find lengths (you can also use strlen)
	while (*f++) ++l1;
	while (*l++) ++l2;

	// allocate a buffer including terminating null char
	char* result = new char[l1 + l2 + 1];

	// then concatenate
	for (int i = 0; i < l1; i++) result[i] = first[i];
	for (int i = l1; i < l1 + l2; i++) result[i] = second[i - l1];

	// finally, "cap" result with terminating null char
	result[l1 + l2] = '\0';
	return result;
}


ObjFile objModel,house,car, road, grass, lavka, dom, grassUnderHouse;
ObjFile beerMan[40];
char fileName[] = "models\\beerMan";
char nameSuffix[] = ".obj";


Texture houseTex, manTex, carTex, roadTex, grassTex, lavkaTex, domTex, grassUnderHouseTex;


//выполняется перед первым рендером
void initRender(OpenGL *ogl)
{

	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);
	
	


	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	//ogl->mainCamera = &WASDcam;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH); 


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	/*
	//texture1.loadTextureFromFile("textures\\texture.bmp");   загрузка текстуры из файла
	*/


	frac.VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	frac.FshaderFileName = "shaders\\frac.frag"; //имя файла фрагментного шейдера
	frac.LoadShaderFromFile(); //загружаем шейдеры из файла
	frac.Compile(); //компилируем

	cassini.VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	cassini.FshaderFileName = "shaders\\cassini.frag"; //имя файла фрагментного шейдера
	cassini.LoadShaderFromFile(); //загружаем шейдеры из файла
	cassini.Compile(); //компилируем
	

	s[0].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[0].FshaderFileName = "shaders\\light.frag"; //имя файла фрагментного шейдера
	s[0].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[0].Compile(); //компилируем

	s[1].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[1].FshaderFileName = "shaders\\textureShader.frag"; //имя файла фрагментного шейдера
	s[1].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[1].Compile(); //компилируем

	

	 //так как гит игнорит модели *.obj файлы, так как они совпадают по расширению с объектными файлами, 
	 // создающимися во время компиляции, я переименовал модели в *.obj_m
	

	

	glActiveTexture(GL_TEXTURE0);
	for (int i = 0; i < 40; i ++) {
		std::string val = std::to_string(i);
		char const* number = val.c_str();
		
		char* suffix = con(number, nameSuffix);
		char* file = con(fileName, suffix);
		loadModel(file, &beerMan[i]);
	}

	loadModel("models\\grass1.obj", &objModel);
	loadModel("models\\Houssobj.obj", &house);
	houseTex.loadTextureFromFile("textures\\house2.bmp");
	houseTex.bindTexture();

	loadModel("models\\road.obj", &road);
	roadTex.loadTextureFromFile("textures\\roadTexjpeg.bmp");
	roadTex.bindTexture();

	loadModel("models\\lavka.obj", &lavka);
	lavkaTex.loadTextureFromFile("textures\\lavkaTex.bmp");
	lavkaTex.bindTexture();

	loadModel("models\\grass5.obj", &grass);
	grassTex.loadTextureFromFile("textures\\grassTex.bmp");
	grassTex.bindTexture();

	loadModel("models\\Car.obj", &car);
	carTex.loadTextureFromFile("textures\\carTex.bmp");
	carTex.bindTexture();

	loadModel("models\\cottage_blender.obj", &dom);
	domTex.loadTextureFromFile("textures\\cottage_diffuse1.bmp");
	domTex.bindTexture();

	loadModel("models\\grassUnderHouse.obj", &grassUnderHouse);
	grassUnderHouseTex.loadTextureFromFile("textures\\grassUnderHouse.bmp");
	grassUnderHouseTex.bindTexture();

	manTex.loadTextureFromFile("textures\\manText.bmp");
	manTex.bindTexture();


	//loadModel("models\\grass.obj", &house);

	tick_n = GetTickCount();
	tick_o = tick_n;

	rec.setSize(700, 100);
	rec.setPosition(10, ogl->getHeight() - 100-10);
	rec.setText("T - вкл/выкл текстур\nL - вкл/выкл освещение\nF - Свет из камеры\nG - двигать свет по горизонтали\nG+ЛКМ двигать свет по вертекали\nM - Экстра раунд",0,0,0);
	//rec.setText("a",0,0,0);
	
}

void keyPressed(unsigned char key, int x, int y) { // обработка нажатий на клавиатуру
}

//double carYpos = 150.0;
double secondCarYpos = 100.0;
double carYpos[] = { 150.0, 100.0, 150.0, 200.0 };


void Render(OpenGL *ogl)
{   
	
	tick_o = tick_n;
	tick_n = GetTickCount();
	Time += (tick_n - tick_o) / 1000.0;

	/*
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	*/

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (lightMode)
		glEnable(GL_LIGHTING);

	//альфаналожение
	/*glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/

	//настройка материала
	GLfloat amb[] = { 0.2, 0.2, 0.1, 1. };
	GLfloat dif[] = { 0.4, 0.65, 0.5, 1. };
	GLfloat spec[] = { 0.9, 0.8, 0.3, 1. };
	GLfloat sh = 0.1f * 256;

	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	//размер блика
	glMaterialf(GL_FRONT, GL_SHININESS, sh);

	s[0].UseShader();

	//передача параметров в шейдер.  Шаг один - ищем адрес uniform переменной по ее имени. 
	int location = glGetUniformLocationARB(s[0].program, "light_pos");
	//Шаг 2 - передаем ей значение
	glUniform3fARB(location, light.pos.X(), light.pos.Y(),light.pos.Z());

	location = glGetUniformLocationARB(s[0].program, "Ia");
	glUniform3fARB(location, 0.2, 0.2, 0.2);

	location = glGetUniformLocationARB(s[0].program, "Id");
	glUniform3fARB(location, 1.0, 1.0, 1.0);

	location = glGetUniformLocationARB(s[0].program, "Is");
	glUniform3fARB(location, 0.7, 0.7, 0.7);


	location = glGetUniformLocationARB(s[0].program, "ma");
	glUniform3fARB(location, 1, 1, 1);

	location = glGetUniformLocationARB(s[0].program, "md");
	glUniform3fARB(location, 1, 1, 1);

	location = glGetUniformLocationARB(s[0].program, "ms");
	glUniform4fARB(location, 1, 1, 1, 25.6);

	location = glGetUniformLocationARB(s[0].program, "camera");
	glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());

	/*int l = glGetUniformLocationARB(s[0].program, "tex");
	glUniform1iARB(l, 0);*/
	
	location = glGetUniformLocationARB(s[0].program, "iViewMatrix");
	float iv_matrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, iv_matrix);
	glUniformMatrix4fv(location, 1, false, iv_matrix);

	//трава
	//objModel.DrawObj();
	/*int l = glGetUniformLocationARB(s[0].program, "tex");
	glUniform1iARB(l, 0); */

	
	


	float time = Time;
	//Shader::DontUseShaders();
	
	

	//grassUnderHouse
	glPushMatrix();
	glTranslated(51.5, 0, 0);
	glRotated(90, 0, 0, 1);
	grassUnderHouseTex.bindTexture();
	grassUnderHouse.DrawObj();
	glPopMatrix();

	if (idioticMode) {
		//тачки
		glPushMatrix();
		if (carYpos[0] < -150) {
			carYpos[0] = 150.0;
		}
		carYpos[0] -= 1.2;


		glTranslated(5 * sin(time * 4) - 50, carYpos[0], 0);
		glRotated(90, 0, 0, 1);
		carTex.bindTexture();
		car.DrawObj();

		glPopMatrix();

		glPushMatrix();
		if (carYpos[1] < -100) {
			carYpos[1] = 100.0;
		}
		carYpos[1] -= 1.6;


		glTranslated(5 * sin(time * 4) - 170, carYpos[1], 0);
		glRotated(90, 0, 0, 1);
		car.DrawObj();

		glPopMatrix();

		glPushMatrix();
		if (carYpos[2] < -150) {
			carYpos[2] = 150.0;
		}
		carYpos[2] -= 1.6;


		glTranslated(5 * sin(time * 4) - 290, carYpos[2], 0);
		glRotated(90, 0, 0, 1);
		car.DrawObj();

		glPopMatrix();

		glPushMatrix();
		if (carYpos[3] < -200) {
			carYpos[3] = 200.0;
		}
		carYpos[3] -= 1.6;


		glTranslated(5 * sin(time * 4) - 410, carYpos[3], 0);
		glRotated(90, 0, 0, 1);
		car.DrawObj();

		glPopMatrix();
	}
	else {
		//тачки
		glPushMatrix();
		if (carYpos[0] < -150) {
			carYpos[0] = 150.0;
		}
		carYpos[0] -= 1.2;


		glTranslated(-50, carYpos[0], 0);
		glRotated(90, 0, 0, 1);
		carTex.bindTexture();
		car.DrawObj();

		glPopMatrix();

		glPushMatrix();
		if (carYpos[1] < -100) {
			carYpos[1] = 100.0;
		}
		carYpos[1] -= 1.6;


		glTranslated(-170, carYpos[1], 0);
		glRotated(90, 0, 0, 1);
		car.DrawObj();

		glPopMatrix();

		glPushMatrix();
		if (carYpos[2] < -150) {
			carYpos[2] = 150.0;
		}
		carYpos[2] -= 1.6;


		glTranslated(-290, carYpos[2], 0);
		glRotated(90, 0, 0, 1);
		car.DrawObj();

		glPopMatrix();

		glPushMatrix();
		if (carYpos[3] < -200) {
			carYpos[3] = 200.0;
		}
		carYpos[3] -= 1.6;


		glTranslated(-410, carYpos[3], 0);
		glRotated(90, 0, 0, 1);
		car.DrawObj();

		glPopMatrix();
	}
	

	//дом
	glPushMatrix();
	glTranslated(-490, 0, 0);
	glRotated(90, 0, 0, 1);
	houseTex.bindTexture();
	house.DrawObj();
	glPopMatrix();

	////дом2
	glPushMatrix();
	glTranslated(70, 0, 0);
	glRotated(0, 0, 0, 0);
	domTex.bindTexture();
	dom.DrawObj();
	glPopMatrix();

	//мужичок

	//s[1].UseShader();
	//int l = glGetUniformLocationARB(s[1].program,"tex"); 
	//glUniform1iARB(l, 0);     //так как когда мы загружали текстуру грузили на GL_TEXTURE0

	glPushMatrix();
	glRotated(90, 0,0, 1);
	

	glTranslated(manPosition[0], manPosition[1], manPosition[2]);

	manTex.bindTexture();
	

	int i = 0;
	int frameNumber;
	std::string a = "";
	
	frameNumber = floorf(fmod(time, 1.00001) * 40.0 / 1.0);
	a = std::to_string(float(int(time) % 3 * 40 / 3));

	char const* number  = a.c_str();
	
	
	beerMan[frameNumber].DrawObj();

	if (manPosition[0] >= abs(carYpos[0]) - 30 && manPosition[0] <= abs(carYpos[0]) + 50 ) {
		if (manPosition[1] >= 30 && manPosition[1] <= 70) {
			manPosition[1] = -10;
		}
	}

	if (manPosition[0] >= abs(carYpos[1]) - 30 && manPosition[0] <= abs(carYpos[1]) + 50) {
		if (manPosition[1] >= 150 && manPosition[1] <= 165) {
			manPosition[1] = -10;
		}
	}

	if (manPosition[0] >= abs(carYpos[2]) - 40 && manPosition[0] <= abs(carYpos[2]) + 60) { // ЭТА НЕ СБИВАЕТ
		if (manPosition[1] >= 170 && manPosition[1] <= 220) {
			manPosition[1] = -10;
		}
	}

	if (manPosition[0] >= abs(carYpos[3]) - 30 && manPosition[0] <= abs(carYpos[3]) + 50) {
		if (manPosition[1] >= 290 && manPosition[1] <= 330) {
			manPosition[1] = -10;
		}
	}

	
	glPopMatrix();


	


	//дороги
	glPushMatrix();
	glTranslated(-50, 0, 0);
	glRotated(90, 0, 0, 1);
	roadTex.bindTexture();
	road.DrawObj();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-170, 0, 0);
	glRotated(90, 0, 0, 1);
	roadTex.bindTexture();
	road.DrawObj();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-290, 0, 0);
	glRotated(90, 0, 0, 1);
	roadTex.bindTexture();
	road.DrawObj();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-410, 0, 0);
	glRotated(90, 0, 0, 1);
	roadTex.bindTexture();
	road.DrawObj();
	glPopMatrix();

	//лавки
	glPushMatrix();
	glTranslated(0, 30, 2);
	glRotated(-90, 0, 0, 2);
	lavkaTex.bindTexture();
	lavka.DrawObj();
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, -30, 2);
	glRotated(90, 0, 0, 2);
	lavkaTex.bindTexture();
	lavka.DrawObj();
	glPopMatrix();

	glPushMatrix();
	glTranslated(20, 0, 2);
	glRotated(180, 0, 0, 2);
	lavkaTex.bindTexture();
	lavka.DrawObj();
	glPopMatrix();


	//трава
	glPushMatrix();
	glTranslated(-110, 0, 0);
	glRotated(90, 0, 0, 1);
	grassTex.bindTexture();
	grass.DrawObj();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-230, 0, 0);
	glRotated(90, 0, 0, 1);
	grassTex.bindTexture();
	grass.DrawObj();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-350, 0, 0);
	glRotated(90, 0, 0, 1);
	grassTex.bindTexture();
	grass.DrawObj();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-470, 0, 0);
	glRotated(90, 0, 0, 1);
	grassTex.bindTexture();
	grass.DrawObj();
	glPopMatrix();
	
	

	//Shader::DontUseShaders();
}   //конец тела функции


bool gui_init = false;

//рисует интерфейс, вызывется после обычного рендера
void RenderGUI(OpenGL *ogl)
{
	
	Shader::DontUseShaders();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_LIGHTING);
	

	glActiveTexture(GL_TEXTURE0);
	rec.Draw();


		
	Shader::DontUseShaders(); 



	
}

void resizeEvent(OpenGL *ogl, int newW, int newH)
{
	rec.setPosition(10, newH - 100 - 10);
}

