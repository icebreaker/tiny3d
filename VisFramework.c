/*
	Copyright (C) 2007, Mihail Szabolcs
                                              
	This program is free software; you can redistribute it and/or             
	modify it under the terms of the GNU General Public License               
	as published by the Free Software Foundation; either version 2            
	of the License, or (at your option) any later version.                    
                                                                           
	This program is distributed in the hope that it will be useful,           
	but WITHOUT ANY WARRANTY; without even the implied warranty of            
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                       
                                                                            
	See the GNU General Public License for more details.            
*/
#include "VisFramework.h"
#include "demo.h"

/* DEMO PROTOTYPES */
void initScene(winampGLVisualizer *this_inst);
void shutdownScene();
void renderScene(struct winampVisModule *this_mod);


void drawHeart(float x, float y, float z, bool smaller, bool rotate, int num);
/* DEMO PROTOTYPES */

void visConfig(struct winampVisModule *this_mod)
{
	//TOOD: add specific configuration stuff here (e.g show an nice dialog, etc)
	MessageBox(this_mod->hwndParent,"Nothing to configure.","Configuration",MB_OK | MB_ICONINFORMATION);	
}

int visInit(struct winampVisModule *this_mod)
{
	// init Win32 stuff
	int styles; // our Window styles
	HWND parent = NULL; // our parent window's handle
	WNDCLASS wc; // our Window class
	HWND (*e)(embedWindowState *v);

	// OpenGL pixel format related
	PIXELFORMATDESCRIPTOR pfd;
	int nPixelFormat;

	getVisInstance()->myWindowState.flags |= EMBED_FLAGS_NOTRANSPARENCY; 
  
	getVisInstance()->myWindowState.r.left		= 0;
	getVisInstance()->myWindowState.r.top		= 0;
	getVisInstance()->myWindowState.r.right		= VIS_SCENE_WIDTH;
	getVisInstance()->myWindowState.r.bottom	= VIS_SCENE_HEIGHT;
     
	*(void**)&e = (void *)SendMessage(this_mod->hwndParent,WM_WA_IPC,(LPARAM)0,IPC_GET_EMBEDIF);

	if (!e)
	{
		MessageBox(this_mod->hwndParent,"This plugin requires Winamp 5.0+.","Error",MB_OK | MB_ICONERROR);
		return 1;
	}

	parent = e(&getVisInstance()->myWindowState);

	SetWindowText(getVisInstance()->myWindowState.me, this_mod->description);
	
	memset(&wc,0,sizeof(wc));
	wc.lpfnWndProc = WndProc;
	wc.hInstance = this_mod->hDllInstance;
	wc.lpszClassName = VIS_USER_CLASS;
	
	if (!RegisterClass(&wc)) 
	{
		MessageBox(this_mod->hwndParent,"Error registering window class.","Error",MB_OK | MB_ICONERROR);
		return 1;
	}

	styles = WS_VISIBLE|WS_CHILDWINDOW|WS_OVERLAPPED|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;
	styles|= CS_HREDRAW | CS_VREDRAW | CS_OWNDC; //add more

	getVisInstance()->hWnd = CreateWindowEx(
		0,
		VIS_USER_CLASS,
		NULL,
		styles,
		0,0,
		VIS_SCENE_WIDTH,VIS_SCENE_HEIGHT,
		parent,
		NULL,
		this_mod->hDllInstance,
		0);

	if (!getVisInstance()->hWnd) 
	{
		MessageBox(this_mod->hwndParent,"Error while creating window.","Error",MB_OK | MB_ICONERROR);
		return 1;
	}

	//SetWindowLong(getVisInstance()->hWnd,GWL_USERDATA,(LONG)this_mod); 
	SendMessage(this_mod->hwndParent, WM_WA_IPC, (WPARAM)getVisInstance()->hWnd, IPC_SETVISWND);

	// Enable OpenGL on the created Window
	if (!(getVisInstance()->hDC=GetDC(getVisInstance()->hWnd)))
	{
		return 1;
	}

	memset(&pfd,0,sizeof(pfd));
	pfd.nSize      = sizeof(pfd);
	pfd.nVersion   = 1;
	pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 16; // we use 16bit color depth for better compatibility
	pfd.cDepthBits = 16; // Z-Buffer
	pfd.iLayerType = PFD_MAIN_PLANE;

	nPixelFormat = ChoosePixelFormat(getVisInstance()->hDC, &pfd);
	SetPixelFormat(getVisInstance()->hDC, nPixelFormat, &pfd);

	if (!(getVisInstance()->hRC=wglCreateContext( getVisInstance()->hDC )))
	{
		return 1;
	}
   
	if(!wglMakeCurrent(getVisInstance()->hDC,getVisInstance()->hRC))
	{
		return 1;
	}
	
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// setup the perspective :P
	resizeGLWindow(VIS_SCENE_HEIGHT,VIS_SCENE_WIDTH);

	// show the window
	ShowWindow(parent,SW_SHOWNORMAL);

	initScene( getVisInstance() );

	return 0;
}

int visRender(struct winampVisModule *this_mod)
{
	// start OpenGL rendering
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	//TODO: add your rendering code here
	renderScene( this_mod );

	// stop OpenGL rendering
	glFlush();
	SwapBuffers(getVisInstance()->hDC);

	return 0;
}

void visQuit(struct winampVisModule *this_mod)
{
	shutdownScene();

	// disable OpenGL
	wglMakeCurrent(NULL,NULL);
	wglDeleteContext(getVisInstance()->hRC);
	ReleaseDC(getVisInstance()->hWnd,getVisInstance()->hDC);

	getVisInstance()->hDC = NULL;
	getVisInstance()->hRC = NULL;

	// de-init Win32 stuff
	SendMessage(this_mod->hwndParent, WM_WA_IPC, 0, IPC_SETVISWND);

	// delete our window
	if (getVisInstance()->myWindowState.me) 
	{
		SetForegroundWindow(getVisInstance()->this_mod.hwndParent);
		DestroyWindow(getVisInstance()->myWindowState.me);
	}
	UnregisterClass(VIS_USER_CLASS,this_mod->hDllInstance); // unregister window class
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		//TODO: handle more specific messages as needed

		case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
				case SC_SCREENSAVE:
				case SC_MONITORPOWER:
				return 0;
			}
			break;
		}

		case WM_DESTROY:
		{
			PostQuitMessage(0); 
			return 0;
		}

		case WM_KEYDOWN:
		{
			//winampVisModule *this_mod = (winampVisModule *) GetWindowLong(hWnd,GWL_USERDATA);
			//PostMessage(this_mod->hwndParent,message,wParam,lParam);

			getVisInstance()->keys[ wParam ] = true;
			PostMessage(getVisInstance()->this_mod.hwndParent,message,wParam,lParam);
			return 0;	
		}

		case WM_KEYUP:
		{
			getVisInstance()->keys[ wParam ] = false;
			PostMessage(getVisInstance()->this_mod.hwndParent,message,wParam,lParam);
			return 0;
		}

		case WM_SIZE:
		{
			resizeGLWindow(LOWORD(lParam),HIWORD(lParam));
			return 0;
		}
	}
	return DefWindowProc(hWnd,message,wParam,lParam);
}

/* DEMO IMPLEMENTATION */
void initScene(winampGLVisualizer *this_inst)
{
	glEnable(GL_NORMALIZE);

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, heart_mat);
	
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,1);

	glLightfv(GL_LIGHT0, GL_POSITION, fPos);

	f = LightParam[3]/100.f;
	fAmbient[0]= f;
	fAmbient[1]= f;
	fAmbient[2]= f;
	fAmbient[3]= 0.f;
	glLightfv(GL_LIGHT0, GL_AMBIENT, fAmbient);

	f = LightParam[4]/100.f;	
	fDiffuse[0]= f;
	fDiffuse[1]= f;
	fDiffuse[2]= f;
	fDiffuse[3]= 0.f;
	glLightfv(GL_LIGHT0, GL_DIFFUSE, fDiffuse);

	f = LightParam[5]/100.f;
	fSpecular[0]= f;
	fSpecular[1]= f;
	fSpecular[2]= f;
	fSpecular[3]= 0.f;
	glLightfv(GL_LIGHT0, GL_SPECULAR, fSpecular);

	f = LightParam[6]/100.f;
	fAmbMat[0]= f;
	fAmbMat[1]= f;
	fAmbMat[2]= f;
	fAmbMat[3]= 0.f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, fAmbMat);

	f = LightParam[7]/100.f;
	fDifMat[0]= f;
	fDifMat[1]= f;
	fDifMat[2]= f;
	fDifMat[3]= 1.f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, fDifMat);

	f = LightParam[8]/100.f;
	fSpecMat[0]= f;
	fSpecMat[1]= f;
	fSpecMat[2]= f;
	fSpecMat[3]= 0.f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, fSpecMat);

	fShine = 128 * LightParam[9]/100.f;
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, fShine);

	f = LightParam[10]/100.f;
	fEmission[0]= f;
	fEmission[1]= f;
	fEmission[2]= f;
	fEmission[3]= 0.f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, fEmission);
		
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	font_Create(&g_font, this_inst->hDC, "Webdings", 16);
}

void shutdownScene()
{
	font_Destroy(&g_font);
}

void renderScene(struct winampVisModule *this_mod)
{
	int i,j,k;

	for(i=0;i<MAX_HEARTS;i++)
		g_hearts[i] = 0;

	j=0;k=0;
	for(i=127;i<576;i++)
	{
		if(j > MAX_HEARTS) break;

		if(g_hearts[j] >= 5) //trim
		{
			g_hearts[j] = 5;
		}
		else
		{
			g_hearts[j] += this_mod->spectrumData[0][i] + this_mod->spectrumData[1][i];
		}
		k++;
		if(k==8){ j++; k=0; }
	}

	// draw the V character (left side)
	drawHeart(-7.0f, 1.0f,MAX_DEPTH,false,false,g_hearts[0]);
	drawHeart(-6.5f, 0.3f,MAX_DEPTH,false,false,g_hearts[1]);
	drawHeart(-6.0f,-0.4f,MAX_DEPTH,false,false,g_hearts[2]);
	// (right side)
	drawHeart(-4.5f, 1.0f,MAX_DEPTH,false,false,g_hearts[3]);
	drawHeart(-5.2f, 0.3f,MAX_DEPTH,false,false,g_hearts[4]);

	// draw the A character (left side)
	drawHeart(-3.5f, 1.0f,MAX_DEPTH,false,false,g_hearts[5]);
	drawHeart(-4.0f, 0.3f,MAX_DEPTH,false,false,g_hearts[6]);
	drawHeart(-4.5f,-0.4f,MAX_DEPTH,false,false,g_hearts[7]);
	// (right side)
	drawHeart(-3.1f, 0.3f,MAX_DEPTH,false,false,g_hearts[8]);
	drawHeart(-2.8f,-0.4f,MAX_DEPTH,false,false,g_hearts[9]);

	// draw the L character (vertical)
	drawHeart(-1.8f, 1.0f,MAX_DEPTH,false,false,g_hearts[10]);
	drawHeart(-1.8f, 0.3f,MAX_DEPTH,false,false,g_hearts[11]);
	drawHeart(-1.8f,-0.4f,MAX_DEPTH,false,false,g_hearts[12]);
	// (horizontal)
	drawHeart(-1.0f,-0.4f,MAX_DEPTH,false,false,g_hearts[13]);

	// draw the E character (vertical)
	drawHeart(0.0f, 1.0f,MAX_DEPTH,false,false,g_hearts[14]);
	drawHeart(0.0f, 0.3f,MAX_DEPTH,false,false,g_hearts[15]);
	drawHeart(0.0f,-0.4f,MAX_DEPTH,false,false,g_hearts[16]);
	// (horizontal)
	drawHeart(0.8f, 1.0f,MAX_DEPTH,false,false,g_hearts[17]);
	drawHeart(0.7f, 0.3f,MAX_DEPTH,true ,false,g_hearts[18]); //make the middle one "smaller"
	drawHeart(0.8f,-0.4f,MAX_DEPTH,false,false,g_hearts[19]);

	// draw the R character (vertical - left)
	drawHeart(1.8f, 1.0f,MAX_DEPTH,false,false,g_hearts[20]);
	drawHeart(1.8f, 0.3f,MAX_DEPTH,false,false,g_hearts[21]);
	drawHeart(1.8f,-0.4f,MAX_DEPTH,false,false,g_hearts[22]);

	// draw the R character (vertical - right)
	drawHeart(2.6f, 1.0f,MAX_DEPTH,false,false,g_hearts[23]);
	drawHeart(2.6f, 0.3f,MAX_DEPTH,true ,false,g_hearts[24]);
	drawHeart(2.8f,-0.4f,MAX_DEPTH,true ,false,g_hearts[25]);

	// draw the I character (vertical)
	drawHeart(3.7f, 1.0f,MAX_DEPTH,false,false,g_hearts[26]);
	drawHeart(3.7f, 0.3f,MAX_DEPTH,false,false,g_hearts[27]);
	drawHeart(3.7f,-0.4f,MAX_DEPTH,false,false,g_hearts[28]);

	// draw the A character (left side)
	drawHeart(5.8f, 1.0f,MAX_DEPTH,false,false,g_hearts[29]);
	drawHeart(5.3f, 0.3f,MAX_DEPTH,false,false,g_hearts[30]);
	drawHeart(4.8f,-0.4f,MAX_DEPTH,false,false,g_hearts[31]);
	// (right side)
	drawHeart(6.2f, 0.3f,MAX_DEPTH,false,false,g_hearts[32]);
	drawHeart(6.5f,-0.4f,MAX_DEPTH,false,false,g_hearts[33]);

}

void drawHeart(float x, float y, float z, bool smaller, bool rotate, int num)
{
	static float rotX = 0.0f; int i; float zz = z;
	float c,f,r,g,b;

	for(i=1;i<=num+1;i++)
	{
		glLoadIdentity();
		glTranslatef(x,y,zz);

		//glColor3f(0.5f,0.0f,0.1f); // color
		c = (float) (11.77*-num)/100;
        f = (float) (11.5*-num)/100;

		r =1-f;
		g =0.0f;
        b =c-f;

		glColor3f(r,g,b); // color

		if(smaller)
		{
			glScalef(0.6f,0.6f,0.6f);
		}
		if(rotate)
		{
			glRotatef(rotX,0.0f,1.0f,0.0f);
			rotX += 0.1f;
		}
		else
		{
			glRotatef(0.0f,0.0f,0.0f,0.0f);
			rotX = 0.0f;
		}
		font_Print(&g_font, "Y");

		if(smaller)
		{
			zz += HEART_STEP_SMALL;
		}
		else
		{
			zz += HEART_STEP;
		}
	}
}
/* DEMO IMPLEMENTATION */
