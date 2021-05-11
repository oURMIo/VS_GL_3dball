#include <windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include "glaux.h"
#include <math.h>

#pragma comment( lib, "opengl32.lib" )
#pragma comment( lib, "glu32.lib" )
#pragma comment( lib, "glaux.lib" )

////////////////////////////////////////////////////////////////////////////////////////
BOOL    keys[256];   // Текущее состояние клавиш (кроме системных)


////////////////////////////////////////////////////////////////////////////////////////
// Поле - массив, как для лабиринта
#define NX	12  // количество по горизонтали
#define NZ	12  // количество вглубь
#define CX	1.0	// размер клетки
#define CZ	1.0
int lab[NX][NZ];


////////////////////////////////////////////////////////////////////////////////////////
// Состояние мыши
typedef struct
{
	int down_x;
	int down_y;
	int up_x;
	int up_y;
	int move_x;
	int move_y;
	int down;
	int wheel;
} t_mouse;
static t_mouse mouse = { 0 };


////////////////////////////////////////////////////////////////////////////////////////
// Положение персонажа (относительно поля лабиринта)
typedef struct
{
	float x, y, z, angle;
} t_me;
static t_me me = {NX / 2, 0.0, 0.0, 0.0}; // координата ног: посередине по x, 0.0 от пола
float nash_rost = 1.0;  // высота камеры над плоскостью


////////////////////////////////////////////////////////////////////////////////////////
// состояние камер
typedef struct
{
	float angle; // угол поворота камеры вокруг оси y
	float look_x; // расстояние от камеры до точки взгляда
	float look_y;
	float look_z;
} t_camera;


static t_camera camera[3] = 
{
// камера, поближе к персонажу - начальная позиция
	{
		0.0, 
		25.0, 
		nash_rost,
		25.0 
	},
// камера, идущая позади персонажа - начальная позиция
	{
		0.0, 
		25.0, 
		nash_rost,
		25.0 
	},
// камера, висящая в пространстве - начальная позиция
	{
		0.0, 
		25.0, 
		nash_rost,
		25.0 	
	}
};

// номер текущей камеры (индекс массива camera[] - может быть 0, 1 или 2)
int nomer = 0;
// максимальный номер камеры
int nomerMax = 2;
GLuint	texture[4];	   // для текстур
static float pac_angle = 0.0; // угол поворота персонажа (чтоб его разглядеть)
#define PAC_FREE	0
#define PAC_BACK	1
int pac_look = PAC_FREE;    // 0

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wparam, LPARAM lparam); // прототип функции окна 

int APIENTRY WinMain(       // Главная функция программы. От операционной системы она получает параметры:
					 HINSTANCE hInstance,     //  дескриптор экземпляра приложения – уникальный идентификатор приложения для операционной системы
					 HINSTANCE hPrevInstance, // не используется, оставлено для совместимости
					 LPSTR lpCmdLine,         // параметры в случае запуска из командной строки
					 int nCmdShow)            // режим отображения окна (SW_SHOW)
{
	HWND hWnd;   // создание дескриптора (уникального регистрационного номера для операционной системы) окна 
	WNDCLASSEX wcex = {sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0,
		hInstance, NULL, NULL, (HBRUSH)GetStockObject(WHITE_BRUSH), NULL, (LPCSTR)"My", NULL};
	RegisterClassEx(&wcex); 

	hWnd = CreateWindow((LPCSTR)"My", 
		(LPCSTR)"РУС ENG $%^", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 
		CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL); 
	ShowWindow(hWnd, SW_MAXIMIZE); 
	//ShowWindow(hWnd, nCmdShow);  // Отображение окна
	UpdateWindow(hWnd); // обновляем рабочую область заданного окна, отправляя сообщение WM_PAINT окну

	MSG msg;   // переменная для принятия в нее сообщения от операционной системы

	while(GetMessage(&msg, NULL, 0, 0))  // извлекаем сообщение из очереди, созданной операционной системой 
	{ 
		TranslateMessage(&msg); // обрабатываем сообщения
		DispatchMessage(&msg);  // передаем сообщения обратно операционной системе
		InvalidateRect(hWnd, NULL, FALSE); // сделать запрос на перерисовку экрана
		Sleep(10); // делаем паузу, чтоб не перегружать процессор
	} 
	return (int)msg.wParam;  // Возвращаем операционной системе код выхода из приложения 
}


void SetWindowPixelFormat(HDC hdc)
{
	PIXELFORMATDESCRIPTOR pfd;
	pfd.nSize       = sizeof(PIXELFORMATDESCRIPTOR);	
	pfd.dwFlags   =  PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType     = PFD_TYPE_RGBA;
	pfd.cColorBits     = 32;
	pfd.cRedBits       = 8;
	pfd.cRedShift      = 16;
	pfd.cGreenBits     = 8;
	pfd.cGreenShift    = 8;
	pfd.cBlueBits      = 8;
	pfd.cBlueShift     = 0;
	pfd.cAlphaBits     = 0;
	pfd.cAlphaShift    = 0;
	pfd.cAccumBits     = 64;    
	pfd.cAccumRedBits  = 16;
	pfd.cAccumGreenBits   = 16;
	pfd.cAccumBlueBits    = 16;
	pfd.cAccumAlphaBits   = 0;
	pfd.cDepthBits        = 32;
	pfd.cStencilBits      = 8;
	pfd.cAuxBuffers       = 0;
	pfd.iLayerType        = PFD_MAIN_PLANE;
	pfd.bReserved         = 0;
	pfd.dwLayerMask       = 0;
	pfd.dwVisibleMask     = 0;
	pfd.dwDamageMask      = 0;

	SetPixelFormat( hdc, ChoosePixelFormat( hdc, &pfd), &pfd);
}

void Resize(int width,int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective(40.0, (float)width/height, 1.0, 1.0 + NZ*2);
}


void DrawMe()
{
	GLUquadricObj *quadObj;
	quadObj = gluNewQuadric();
	gluQuadricTexture(quadObj, GL_TRUE);
	gluQuadricDrawStyle(quadObj, GLU_FILL);

	glPushMatrix();
	glTranslatef(me.x, nash_rost*0.5, me.z);
	
	glEnable(GL_TEXTURE_2D);
	glRotatef(pac_angle, 0.0, 1.0, 0.0);
	glRotated(-90, 1,0,0);

	glBindTexture(GL_TEXTURE_2D, texture[2]);
	gluSphere(quadObj, 0.25, 16, 16);
	
	glPopMatrix();
	gluDeleteQuadric(quadObj);

}

/////////////////////////////////////////////////////////////////////////
// ФАЙЛЫ И ТЕКСТУРЫ
/////////////////////////////////////////////////////////////////////////
// Загрузка картинки и конвертирование в текстуру
GLvoid LoadGLTextures()
{
	// Загрузка картинок
	AUX_RGBImageRec *texture1, *texture2, *texture3, *texture4;
	texture1 = auxDIBImageLoad((LPCSTR)"parket1.bmp");  // загрузить изображение в память, связанную с texture1
	
	texture2 = auxDIBImageLoad((LPCSTR)"parket2.bmp");
	texture3 = auxDIBImageLoad((LPCSTR)"pac.bmp");

//	texture4 = auxDIBImageLoad((LPCSTR)"_zal\\belokolonnyi24r.bmp");
	texture4 = auxDIBImageLoad((LPCSTR)"_zal\\gold.bmp");

	// Создание текстуры 1
	glGenTextures(1, &texture[0]);   //  
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, texture1->sizeX, texture1->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture1->data);
	// Создание текстуры 2
	glGenTextures(1, &texture[1]);  // 
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, texture2->sizeX, texture2->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture2->data);
	// Создание текстуры 3
	glGenTextures(1, &texture[2]);  // 
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, texture3->sizeX, texture3->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture3->data);
	// Создание текстуры 4
	glGenTextures(1, &texture[3]);  // 
	glBindTexture(GL_TEXTURE_2D, texture[3]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, texture4->sizeX, texture4->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture4->data);

}


////////// Освещение
void InitLighting()
{
	GLfloat material_diffuse[] = {1.0, 1.0, 1.0, 0.0};
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);

	// фоновый цвет
	GLfloat LightAmbient[4] = {10.0, 10.0, 10.0, 10.0};
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, LightAmbient);
	glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);  
	glEnable(GL_LIGHT0);
}


void Draw(HDC hDC) 
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); // очистка 
	glClearColor(0.0, 0.0, 0.2, 0.0); // цвет очистки экрана

	glMatrixMode(GL_MODELVIEW);  
	glLoadIdentity();

	///////////////////////////////////////////////////////////////////////////
	// для камеры, связанной с персонажем
	if (nomer == 0)
	{
		gluLookAt( me.x, camera[nomer].look_y, me.z,    

				// смотрим в точку на плоскости лабиринта: мы + сдвиг по x и z
				me.x + camera[nomer].look_x * cos(camera[nomer].angle), 
				0.0, 
				me.z + camera[nomer].look_z * sin(camera[nomer].angle),  

				0.0, 1.0, 0.0);  // вертикаль (верх - по оси y); эту часть обычно не надо менять

		// делаем шаг "назад", чтобы видеть себя, свой "затылок" - то есть отодвигаем сцену вперед
		glTranslatef(3.0 * cos(camera[nomer].angle), 0.0, 3.0 * sin(camera[nomer].angle));
	}
	///////////////////////////////////////////////////////////////////////////
	// для камеры позади персонажа
	else if (nomer == 1)
	{
		gluLookAt( me.x, camera[nomer].look_y, me.z,    

				// смотрим в точку на плоскости лабиринта: мы + сдвиг по x и z
				me.x + camera[nomer].look_x * cos(camera[nomer].angle), 
				0.0, 
				me.z + camera[nomer].look_z * sin(camera[nomer].angle),  

				0.0, 1.0, 0.0);  // вертикаль (верх - по оси y); эту часть обычно не надо менять

		// делаем шаг "назад", чтобы видеть себя, свой "затылок" - то есть отодвигаем сцену вперед
		glTranslatef(6.0 * cos(camera[nomer].angle), 0.0, 6.0 * sin(camera[nomer].angle));
	}
	///////////////////////////////////////////////////////////////////////////
	// для камеры, свободно перемещающейся  // else вместо номера - на случай если придёт некорректный номер
	else //if (nomer == 2)
	{
		gluLookAt( me.x, camera[nomer].look_y, me.z,    

				// смотрим в точку на плоскости лабиринта: мы + сдвиг по x и z
				camera[nomer].look_x * cos(camera[nomer].angle), 
				0.0, 
				camera[nomer].look_z * sin(camera[nomer].angle),  

				0.0, 1.0, 0.0);  // вертикаль (верх - по оси y); эту часть обычно не надо менять

	}


	glEnable(GL_TEXTURE_2D);		// Разрешение наложения текстуры
	glEnable(GL_LIGHTING);
	for (int i = 0; i < NX; i++)
		for (int j = 0; j < NZ; j++)
		{
			glColor3f((float)(i+1) / NX, (float)(j+1) / NZ, 0.5);
			if ((i % 2 + j % 2 == 1)) glBindTexture(GL_TEXTURE_2D, texture[0]);
			else glBindTexture(GL_TEXTURE_2D, texture[1]);
			glBegin(GL_QUADS); // начало рисования прямоугольника, z == const
			glTexCoord2f(0.0f, 0.0f); glVertex3f(i*CX,		0.0,	j*CZ);
			glTexCoord2f(1.0f, 0.0f); glVertex3f(i*CX + CX,	0.0,	j*CZ);
			glTexCoord2f(1.0f, 1.0f); glVertex3f(i*CX + CX,	0.0,	j*CZ + CZ);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(i*CX,		0.0,	j*CZ + CZ);
			glEnd();
		}

		
    // картина		
	glBindTexture(GL_TEXTURE_2D, texture[3]);
	glBegin(GL_QUADS); // начало рисования прямоугольника, z == const

	glTexCoord2f(0.0f, 0.0f); glVertex3f(NX, 0.0, 0.0);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(NX, 0.0, NZ);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(NX, 8.0, NZ);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(NX, 8.0, 0.0);
	glEnd();

		
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);


	// синяя рамочка вокруг пола
	glColor3f(0.0, 0.0, 1.0);
	glLineWidth(3); 
	glBegin(GL_LINE_LOOP);
	glVertex3f(-1, 0.0, -1.0);
	glVertex3f(NX + 1, 0.0, -1.0);
	glVertex3f(NX + 1, 0.0, NZ + 1);
	glVertex3f(-1, 0.0, NZ + 1);
	glEnd();

	glEnable(GL_ALPHA_TEST); // включаем режим тестирования прозрачности
	glEnable(GL_BLEND); // Разрешение прозрачности
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA );
	// потолок
	glColor4f(1.0, 0.0, 0.0, 0.5);
	glBegin(GL_QUADS);
	glVertex3f(-1, 10.0, -1.0);
	glVertex3f(NX + 1, 10.0, -1.0);
	glVertex3f(NX + 1, 10.0, NZ + 1);
	glVertex3f(-1, 10.0, NZ + 1);
	glEnd();

	// полупрозрачная стенка - отгороженный угол
	glColor4f(1.0, 1.0, 1.0, 0.5);
	glBegin(GL_QUADS);
	glVertex3f(0.0, 0.0, 2.0);
	glVertex3f(0.0, 2.0, 2.0);
	glVertex3f(3.0, 2.0, 0.0);
	glVertex3f(3.0, 0.0, 0.0);
	glEnd();

	glDisable(GL_ALPHA_TEST); // выключаем режим тестирования прозрачности
	glDisable(GL_BLEND); // запрещение прозрачности

	glColor4f(1.0, 1.0, 0.0, 0.75);
	DrawMe();


	SwapBuffers(wglGetCurrentDC()); // переключить буфер
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wparam, LPARAM lparam)  // дескриптор окна, сообщение, два параметра сообщения
{ 
	static HDC hdc;  
	static HGLRC hGLRC;
	if (Message == WM_CREATE) // при создании окна создаем кнопки
	{
		hdc = GetDC(hwnd); // получить дескриптор окна
		SetWindowPixelFormat(hdc); // установить для него настройки
		hGLRC = wglCreateContext(hdc); // создать контекст OpenGL
		wglMakeCurrent(hdc, hGLRC); // сделать этот контекст OpenGL текущим 
		glEnable(GL_DEPTH_TEST); // включаем режим тестирования глубины

		// включение освещения
		InitLighting();
		glEnable(GL_LIGHTING);

		// Загрузка текстур
		LoadGLTextures();

		// Поставим себя в центр
		me.x = NX / 2;
		me.z = NZ / 2;
	}
	if (Message == WM_SIZE ) 
	{
		Resize( LOWORD(lparam), HIWORD(lparam) );
	}
	if (Message == WM_PAINT)
	{
		Draw(hdc); // прикладная функция - рисование кадра
	}
	if (Message == WM_KEYDOWN)
	{
		keys[wparam] = true;
		// вперед и назад
		if (keys[VK_UP]) 
		{
			me.x += 1.0 * cos(me.angle);
			me.z += 1.0 * sin(me.angle);
		}
		if (keys[VK_DOWN]) 
		{
			me.x -= 1.0 * cos(me.angle);
			me.z -= 1.0 * sin(me.angle);
		}

		// поворот
		if (keys[VK_LEFT]) 
		{
			me.angle -= 0.1;
			if (me.angle < 0.0) me.angle += 6.28;

			camera[0].angle = me.angle;
			camera[1].angle = me.angle;
		}
		if (keys[VK_RIGHT]) 
		{
			me.angle += 0.1;
			if (me.angle > 6.28) me.angle -= 6.28;

			camera[0].angle = me.angle;
			camera[1].angle = me.angle;
		}
		// движение камеры по вертикали
		if (keys['W']) camera[nomer].look_y += 1.0; 
		if (keys['S']) camera[nomer].look_y -= 1.0;

		if (nomer == 2)
		{
			if (keys['A']) camera[nomer].look_y += 1.0; 
			if (keys['D']) camera[nomer].look_y -= 1.0;
		}

		if (keys[VK_ESCAPE]) 
		{
			PostQuitMessage(0);
		}
		if (keys[VK_SPACE]) 
		{
			nomer++;
			if (nomer > nomerMax) 
				nomer = 0;
		}
		if (keys[VK_RETURN])
		{
			;
		}
		if ((keys['p'] == 1) || (keys['P'] == 1) || (keys['з'] == 1) || (keys['З'] == 1))
		{
			pac_look = (PAC_FREE == pac_look) ? PAC_BACK : PAC_FREE;
		}

		// независимо от клавиши синхронизируем угол поворота персонажа
		if (PAC_FREE == pac_look)
		{
			pac_angle = 180.0;
		}
		else
		{
			pac_angle = -camera[nomer].angle * 180 / 3.1415927;  //  + 180.0;  видеть спину или лицо?
		}
	}
	if (Message == WM_KEYUP)
	{
		keys[wparam] = false;
	}
	if (Message == WM_LBUTTONDOWN)
	{
		mouse.down_x = LOWORD(lparam); 
		mouse.down_y = HIWORD(lparam); 
		mouse.down = 1;
	}
	if (Message == WM_MOUSEMOVE)
	{
		mouse.move_x = LOWORD(lparam); 
		mouse.move_y = HIWORD(lparam); 
		if (mouse.down == 1)
		{
			mouse.down_x = mouse.move_x;
			mouse.down_y = mouse.move_y;
		}
	}
	if (Message == WM_LBUTTONUP)
	{
		mouse.up_x = LOWORD(lparam); 
		mouse.up_y = HIWORD(lparam); 
		mouse.down = 0;
	}

	if (Message == WM_DESTROY )  // если операционная система требует завершить работу (приняла нажатие Alt-f4, кнопки в заголовке и т.д.)
	{ 
		PostQuitMessage(0); // Сообщить операционной системе о нашем желании закрыть приложение
	} 
	return DefWindowProc(hwnd, Message, wparam, lparam); // перед выходом вызвать стандартный обработчик не проработанных нами сообщений
}

