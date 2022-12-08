/*************************************************************
 CAD Systems Project. Elias Cohenca. Student Number: 015257231
 
 NOTES: Use the up & down directional keys to zoom in & out
		Select a gear with Ctrl key pressed to make it 
		transparent
		Use mouse left-button to rotate model. Rotating with shift 
		key	pressed makes the model continue the rotation after
		releasing the mouse button
		'S' & 'D' (caps-independent) animates the gears & model
		'O'(also caps-independent) changes between orthographic
		and perspective views
		Right-clicking brings up the options menu, from here
		you can change the gear configuration and turn on/off
		the lights. There's also a reinitialize option to go
		back to the initial default states
		First light is located on the model and rotates with it
		Second light comes straight from the viewer and it's fixed
		The third light is a spotlight located in the upper-right
		corner (and slightly in front) of the screen 
*************************************************************/


#include <stdio.h>
#include <math.h>
#include <windows.h>		//For booleans
#include <GL\glut.h>

#define MAX_VERTICES 1000
#define MAX_FACES 2000
#define MAX_POINTS_PER_FACE 4
#define BUFSIZE 100
#define BUFPICKING 64			//Buffer for picking function
#define ZOOM_FACTOR 1.02
#define M_PI 3.141592654 
#define SMALL1 1			//Object name list for picking function
#define LARGE2 2
#define SMALL3 3
#define LARGE4 4
#define SMALL5 5
#define LARGE6 6




typedef double vect3D[3];
typedef int vecti[MAX_POINTS_PER_FACE];

typedef struct MODEL{
	int numberOfPoints;  
	int numberOfFaces;		
	vect3D points[MAX_VERTICES];	//vertices list (each [i] has 3 points)
	vecti faces[MAX_FACES];			//faces list (each [i] has a list of 3 vertices)
									
	int pointsPerFace[MAX_FACES];
	vect3D faceNormal[MAX_FACES];
	vect3D vertexNormal[MAX_VERTICES];

} Model;

enum PROJ {ORTHO, PERSPECTIVE};

void createMenu();
void drawScene();
void picked(int x, int y);
void findFaceNormal(Model *M);


Model Fixer;			/*Initialize objects*/
Model CW_Small;
Model CW_Large;

double or, ol, ot, ob;	/*Ortho values*/

float AR;                /*Aspect Ratio*/

int sub1 = 1;	/*Default config menu value: First config*/


PROJ projection = ORTHO;	/*Default projection: Ortho*/


BOOL switch0 = TRUE;	/*Light switches*/
BOOL switch1 = TRUE;
BOOL switch2 = TRUE;

double angle = 0.0, axis[3];		/*Rotation, axis of rotation and scaling values*/
float scale = 1.0;

BOOL do_rotate = FALSE;		/*Condition for rotate*/
BOOL tracking = FALSE;		/*Are we still tracking the mouse movement?*/


BOOL shiftState = FALSE;
BOOL ctrlState = FALSE;

BOOL alpha_part[7];		/*Keeps track of objects (if they are transparent or not)*/


vect3D a;		/*mouse position on 'trackball' sphere*/
vect3D b;
GLdouble rotTemp[16];		/*Saves the modelview matrix(to keep track of previous rotations)*/

int do_animate=0;      /*Animation direction (forward or backward)*/
double animation=0.0;	/*Angular change for animation*/



/*******************Functions for vector operations**************************/
void vec_init(vect3D v, double x, double y, double z)
{
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

double vec_module(vect3D v)
{
	return (double)sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

void vec_normal(vect3D v)
{
	double module = vec_module(v);
	if(module == 0.0) module = 1.0;

	v[0] /= module;
	v[1] /= module;
	v[2] /= module;
}

void vec_copy(vect3D v1, vect3D v2)
{
	int i=0;
	for(i=0; i<3; i++)
		v2[i] = v1[i];
}

void vec_add(vect3D v1, vect3D v2, vect3D result)
{
	int i=0;
	for(i=0; i<3; i++)
		result[i] = v2[i] + v1[i];
}

void vec_rest(vect3D v1, vect3D v2, vect3D result)
{
	int i=0;
	for(i=0; i<3; i++)
		result[i] = v2[i] - v1[i];
}

double vec_dot(vect3D v1, vect3D v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void vec_cross(vect3D v1, vect3D v2, vect3D result)
{
	result[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
	result[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
	result[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
}

/**********************************************************************/


/********************Reading file function*****************************/
int ReadOffFile(const char* filename, Model* model)
{
	char buf[BUFSIZE];
	int numberOfPoints, numberOfFaces, i, ptsPrFace, tmp;
	double x,y,z;
	FILE* inp=fopen(filename,"r");
	if(inp==NULL) return 0;

	fgets(buf,BUFSIZE,inp);
	fgets(buf,BUFSIZE,inp);
	sscanf(buf,"%d %d",&numberOfPoints,&numberOfFaces);
	if(numberOfPoints>MAX_VERTICES || numberOfFaces>MAX_FACES){
		fclose(inp);
		printf("Amount of vertices or faces exceeds the MAXIMUM\n");
		return 0;
	}

	for(i=0;i<numberOfPoints;i++){
		fgets(buf,BUFSIZE,inp);
		sscanf(buf,"%lf %lf %lf",&x,&y,&z);
		model->points[i][0]=x;
		model->points[i][1]=y;
		model->points[i][2]=z;
	}

	for(i=0;i<numberOfFaces;i++){
		fgets(buf,BUFSIZE,inp);
		sscanf(buf,"%d",&ptsPrFace);
		if(ptsPrFace>MAX_POINTS_PER_FACE || ptsPrFace<3){
			fclose(inp);
			printf("Too much/few vertices per face\n");
			return 0;
		}

		switch(ptsPrFace){
			case 3:
				sscanf(buf,"%d %d %d %d",&tmp,&model->faces[i][0],&model->faces[i][1],&model->faces[i][2]);
				model->pointsPerFace[i]=tmp;
				break;
			case 4:
				sscanf(buf,"%d %d %d %d %d",&tmp,&model->faces[i][0],&model->faces[i][1],&model->faces[i][2],&model->faces[i][3]);
				model->pointsPerFace[i]=tmp;
				break;
			default:
				return 0;
		}
	}

	model->numberOfPoints=numberOfPoints;
	model->numberOfFaces=numberOfFaces;

	fclose(inp);
	return 1;
}
/********************************************************************/

/*****Process hits from the picking function*****/
void processHits (GLint hits, GLuint buffer[])
{
  	int i;
	int choose, depth;
	
	
	if (hits > 0)								
	{
		choose = buffer[3];					// Make selection the first object
		depth = buffer[1];					// Store how far away it is


		for (i = 1; i < hits; i++)		//Compare to other objects. If it's closer, replace
		{
			
			if (buffer[i*4+1] < GLuint(depth))
			{
				choose = buffer[i*4+3];			
				depth = buffer[i*4+1];			
			}       
		}
	
		alpha_part[choose] =! alpha_part[choose]; //Change the status(transparency) of the object
												  //that was selected														
				
	}
  
}


/***Makes 2 vectors out of 3 points in a face to find the normal using the fact
that cross multiplication gives a perpendicular vector***/
void findFaceNormal(Model *M)
{
	int i, j;
	vect3D a, b, c, v1, v2;

	
	for(i=0; i<M->numberOfFaces; i++)
	{
		for(j=0; j<M->pointsPerFace[i]; j++)
				{
			a[j]=M->points[M->faces[i][0]][j];	//3 points defining the face
			b[j]=M->points[M->faces[i][2]][j];  //Points are in inversed order (0, 2, 1)
			c[j]=M->points[M->faces[i][1]][j];  //because the off file is in CW order
		}
		vec_rest(b, a, v1);				
		vec_rest(c, a, v2);
		vec_cross(v1, v2, M->faceNormal[i]);
		vec_normal(M->faceNormal[i]);

	}
}

/***Finds normal per vertex by adding the normals from the surrounding faces
and normalizing the resulting normal***/		
void findVertexNormal(Model *M)
{
	int i, j, k;
	vect3D temp = {0.0, 0.0, 0.0};

	for(i=0; i<M->numberOfPoints; i++)
	{
		for(j=0; j<M->numberOfFaces; j++)
			for(k=0; k<M->pointsPerFace[j]; k++)
			{
				if(M->faces[j][k] == i)
				{
					vec_add(M->faceNormal[j], temp, temp);
					k = M->pointsPerFace[j];
				}
			}
		
		vec_normal(temp);
		vec_copy(temp, M->vertexNormal[i]);
		
	}
}

		

/***Object drawing function***/
void draw_obj(Model *M)
{
	int i; 
	
	
	
	for(i=0; i<M->numberOfFaces; i++)
	{
		glBegin(GL_TRIANGLES);	
			if(scale < 1)
			{
				M->faceNormal[i][0]*=scale;			//prevents problems with lighting when scaling
				M->faceNormal[i][1]*=scale;
				M->faceNormal[i][2]*=scale;
			}
			glNormal3dv(M->faceNormal[i]);		//Normal per face
			//glNormal3dv(M->vertexNormal[M->faces[i][0]]);		//Points are in inversed order (0, 2, 1)
			glVertex3dv(M->points[M->faces[i][0]]);				//because the off file is in CW order
			//glNormal3dv(M->vertexNormal[M->faces[i][2]]);
			glVertex3dv(M->points[M->faces[i][2]]);
			//glNormal3dv(M->vertexNormal[M->faces[i][1]]);
			glVertex3dv(M->points[M->faces[i][1]]);
			if(scale < 1)
			{
				M->faceNormal[i][0]/=scale;
				M->faceNormal[i][1]/=scale;
				M->faceNormal[i][2]/=scale;
			}
		glEnd();
	}

}

/***Reshape Callback function***/
void reshapeCB(int width, int height)
{
        glViewport(0, 0, width, height );       /*Defines viewport same as world coordinates*/
        
            
        AR = float(width)/float(height);        /*Defines aspect ratio*/

        if(AR>=1)
        {
                ol=-0.2*AR;
                or=0.2*AR;
                ob=-0.2;
                ot=0.2;
        }
        if(AR<1)                                                                                        
        {
                ol=-0.2;
                or=0.2;
                ob= -0.2/AR;
                ot=0.2/AR;
		}

		glMatrixMode(GL_PROJECTION);                
		glLoadIdentity();

		if(projection == ORTHO)
			glOrtho(ol, or, ob, ot, -5.0, 5.0);
		
	
		if(projection == PERSPECTIVE)
		    gluPerspective(30, AR, 0.1, 5);
	
}


/***Defines projection, lighting, and basically puts everything in screen. Put this here
instead of in the drawing callback because the picking function needs to draw everything
to know what object was selected, so this way we can call the drawing functions without
actually sending to the drawing buffer***/
void drawScene()
{
	
	glMatrixMode(GL_MODELVIEW);                        
	glLoadIdentity();

	if(projection == ORTHO)
	{
        glPushMatrix();
	}
	
	if(projection == PERSPECTIVE)
	{
        glPushMatrix();
		gluLookAt(0, 0, 0.768, 0, 0, 0, 0, 1, 0);
	}

	

	glCullFace(GL_BACK);		//Don't draw polygons in the back
	glEnable(GL_CULL_FACE);

	glShadeModel(GL_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	
	glDepthFunc(GL_LEQUAL);		/*For z-buffer*/
	glEnable(GL_DEPTH_TEST);	
	
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);	/*For transparency*/
	glEnable(GL_BLEND);		
	glAlphaFunc(GL_GREATER,0.1f);
	glEnable(GL_ALPHA_TEST);
    	
	glInitNames();   //Name stack for picking
	glPushName(0);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f );      
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
		
	GLfloat global_ambient[4]={0.7, 0.7, 0.7, 1.0};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

		
	//Viewer-centered light
	GLfloat position1[4]={0.0, 0.0, 1.2, 1.0};
	GLfloat ambient1[4]={0.5, 0.5, 0.5, 1.0};
	GLfloat diffuse1[4]={0.8, 0.8, 0.8, 1.0};
	GLfloat specular1[4]={0.2, 0.2, 0.2, 1.0};
	GLfloat spotdir1[4]={0.005, 0.005, 0.0, 1.0};
		
	glLightfv(GL_LIGHT1, GL_POSITION, position1);
	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse1);
	glLightfv(GL_LIGHT1, GL_SPECULAR, specular1);
	glLightfv(GL_LIGHT1 ,GL_SPOT_DIRECTION, spotdir1);
	glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 120.0);
	if(switch1 == TRUE) glEnable(GL_LIGHT1);
	else if(switch1 == FALSE) glDisable(GL_LIGHT1);
	
	//Extra spotlight
	GLfloat position2[4]={0.1, 0.05, 0.075, 1.0};
	GLfloat ambient2[4]={0.2, 0.2, 0.6, 1.0};
	GLfloat diffuse2[4]={0.2, 0.2, 0.6, 1.0};
	GLfloat specular2[4]={0.8, 0.8, 0.6, 1.0};
	GLfloat spotdir2[4]={0.025, 0.0, 0.0, 1.0};
	
	glLightfv(GL_LIGHT2, GL_POSITION, position2);
	glLightfv(GL_LIGHT2, GL_AMBIENT, ambient2);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, diffuse2);
	glLightfv(GL_LIGHT2, GL_SPECULAR, specular2);
	glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.5);
	glLightfv(GL_LIGHT2 ,GL_SPOT_DIRECTION, spotdir2);
	glLightf(GL_LIGHT2, GL_SPOT_EXPONENT,50.0);
	glLightf(GL_LIGHT2, GL_SPOT_CUTOFF ,150.0);
	if(switch2 == TRUE) glEnable(GL_LIGHT2);
	else if(switch2 == FALSE) glDisable(GL_LIGHT2);
	

	/***This is for rotation. We do a rotation, then save the matrix into rotTemp, and multiply
	it the next time to the current rotation matrix. This way we keep track of previous rotations
    */
	
	if(angle >= 360.0) angle-= 360.0;
	if(angle <= -360.0) angle+= 360.0;
		
	glRotated(angle, axis[0], axis[1], axis[2]);
	glMultMatrixd(rotTemp);
	glScalef(scale, scale, scale);

	if(tracking == TRUE)
	{
	
		glPushMatrix();
		glLoadIdentity();
		glRotated(angle, axis[0], axis[1], axis[2]);
		glMultMatrixd(rotTemp);
		glGetDoublev(GL_MODELVIEW_MATRIX, rotTemp);
		glPopMatrix();
	}
	if(shiftState) tracking = TRUE;	  //This so it keeps rotating after releasing the button (with shift on)
		else tracking = FALSE;
	


	/******************************/
	/*Model axis-centered light****/
	glPushMatrix();
	glRotatef(animation, 0, 0, 1);		//Rotate with the model when it's animated

	GLfloat position0[4]={0.07, 0.08, 0.01, 1.0};
	GLfloat ambient0[4]={0.4, 0.15, 0.15, 1.0};
	GLfloat diffuse0[4]={1.0, 0.7, 0.7, 1.0};
	GLfloat specular0[4]={0.1, 0.1, 0.1, 1.0};
	
	glLightfv(GL_LIGHT0, GL_POSITION, position0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular0);
	if(switch0 == TRUE) glEnable(GL_LIGHT0);
	else if(switch0 == FALSE) glDisable(GL_LIGHT0);
	
	glPopMatrix();
	/******************************/
	
	
	/**Material color & properties definitions**/
	//Steel-ish
	GLfloat mat_ambient0[] = {0.23125, 0.23125, 0.23125, 1.0};
	GLfloat mat_diffuse0[] = {0.3, 0.3, 0.3, 1.0 };
	GLfloat mat_specular0[] = {0.87, 0.871, 0.87, 1.0};
	GLfloat mat_shininess0[] = { 100.6 };
	
	//Ruby-ish
	GLfloat mat_ambient1[] = { 0.1745, 0.01175, 0.01175, 1.0};
	GLfloat mat_diffuse1[] = {0.61424, 0.04136, 0.04136, 1.0};
	GLfloat mat_diffuse1t[] = { 0.61424, 0.04136, 0.04136, 0.15 };
	GLfloat mat_specular1[] = { 0.727811, 0.626959, 0.626959, 1.0};
	GLfloat mat_shininess1[] = {76.8 };

	//Brass-ish
	GLfloat mat_ambient2[] = {0.329412, 0.223529, 0.027451, 1.0};
	GLfloat mat_diffuse2[] = { 0.780392, 0.568627, 0.113725, 1.0};
	GLfloat mat_diffuse2t[] = { 0.780392, 0.568627, 0.113725, 0.15 };
	GLfloat mat_specular2[] = { 0.992157, 0.941176, 0.807843, 1.0};
	GLfloat mat_shininess2[] = { 27.8974};
	


	//cogwheel configuration
	
	switch(sub1)
	{
	case 1:
		{
		
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient0);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse0);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular0);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess0);
		
		glRotatef(animation, 0, 0, 1);
		draw_obj(&Fixer);
      	
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient1);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse1);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular1);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess1);
		
		glPushMatrix();
		glTranslatef(0.05, 0, 0.012);
		glRotatef(-8*animation, 0, 0, 1);
		if(alpha_part[SMALL1] == TRUE)
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse1t);
		glLoadName(SMALL1);
		draw_obj(&CW_Small);
		glPopMatrix();
	
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient2);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse2);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular2);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess2);
		
		glPushMatrix();
		glTranslatef(0.095, 0, 0.012);
		glRotatef(11.25+5*animation, 0, 0, 1);
		if(alpha_part[LARGE2] == TRUE)
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse2t);
		glLoadName(LARGE2);
		draw_obj(&CW_Large);
		glPopMatrix();
	
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient1);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse1);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular1);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess1);
		
		glPushMatrix();
		glTranslatef(0.14, 0, 0.012);
		glRotatef(-8*animation, 0, 0, 1);
		if(alpha_part[SMALL3] == TRUE)
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse1t);
		glLoadName(SMALL3);
		draw_obj(&CW_Small);
		glPopMatrix();
		do_animate=0;
		break;
		}
	case 2:
		{
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient0);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse0);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular0);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess0);
		
		glRotatef(animation, 0, 0, 1);
		draw_obj(&Fixer);
		
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient2);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse2);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular2);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess2);
		
		glPushMatrix();
		glTranslatef(0.05, 0, 0.012);
		glRotatef(-17*animation/3, 0, 0, 1);
		glColor4ub(134, 151, 232, 255);
		if(alpha_part[LARGE4] == TRUE)
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse2t);
		glLoadName(LARGE4);
		draw_obj(&CW_Large);
		glPopMatrix();
	
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient1);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse1);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular1);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess1);
		
		glPushMatrix();
		glTranslatef(0.095, 0, 0.012);
		glRotatef(18+(136*animation/15), 0, 0, 1);
	    glColor4ub(153, 222, 143, 255);
		if(alpha_part[SMALL5] == TRUE)
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse1t);
		glLoadName(SMALL5);
		draw_obj(&CW_Small);
		glPopMatrix();
	
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient2);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse2);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular2);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess2);
		glPushMatrix();

		glTranslatef(0.14, 0, 0.012);
		glRotatef(-17*animation/3, 0, 0, 1);
		glColor4ub(134, 151, 232, 255);
		if(alpha_part[LARGE6] == TRUE)
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse2t);
		glLoadName(LARGE6);
		draw_obj(&CW_Large);
		glPopMatrix();
		do_animate=0;
		break;
		}
	}
	glPopName();
	
	glEnable(GL_LIGHTING);
	glPopMatrix();
	
	glutSwapBuffers();
}

/***Drawing Callback function***/
void drawingCB()
{
	drawScene();
	glDisable(GL_DEPTH_TEST);
}

/***Picking function. Defines a small box around the mouse's (x,y) position, then draws the screen
without sending to the actual drawing buffer to check what objects are in that area. selectBuff
is a buffer to save the names of the objects, and their Zmin & Zmax positions.
We then send this (and 'hits' which is the amount of objects in area) to processHits, which
finds whose closer to us (so that we can later show it as transparent) ****/
void picked(int x, int y)
{
	GLuint selectBuff[BUFPICKING];
	GLint hits, viewport[4];
	double p[16];

	glSelectBuffer(BUFPICKING, selectBuff);

	glGetIntegerv(GL_VIEWPORT, viewport);

	glMatrixMode(GL_PROJECTION);
	
	glGetDoublev(GL_PROJECTION_MATRIX,p);
	glPushMatrix();
	glLoadIdentity();

	glRenderMode(GL_SELECT);	//Go to selection mode
	
	gluPickMatrix(x, viewport[3] - y, 2, 2, viewport);
	glMultMatrixd(p);
	
	drawScene();
		
	hits = glRenderMode(GL_RENDER);		//Return to normal rendering mode

	processHits(hits, selectBuff);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

}

/***Projects the mouse (x,y) position into a sphere with radius 1.***/
void mouse2sphere(int x, int y, vect3D v)
{
	
	float w = glutGet(GLUT_WINDOW_WIDTH);
    float h = glutGet(GLUT_WINDOW_HEIGHT);

	double d;
	double n = ((w < h) ? w : h);
	
	v[0] = (double)(2*x)/n - 1.0;	//Transform xy mouse coords into a circle with radius 1
	v[1] = 1.0 - (double)(2*y)/n;
	
	d = v[0]*v[0] + v[1]*v[1];		//Distance on xy plane

	if(d > 1) d = 1;				//Make sure we stay inside the circumference!

	v[2] = (1 - d);
	
	v[2] = ((v[2] > 0) ? sqrt(v[2]) : 0);	//Make sure we don't take the square root of a negative

	vec_normal(v);   
}

/*Callback for mouse buttons*/
void mouseCB(int button, int state, int x, int y)
{
	

	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		do_rotate = TRUE;
		tracking = FALSE;
		mouse2sphere(x, y, a);	//Take the first point for rotation
		shiftState = FALSE;
	
	}
	if(button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
	
		do_rotate = FALSE;
		if(glutGetModifiers() == GLUT_ACTIVE_SHIFT)
		{
			tracking = TRUE;
			shiftState = TRUE;
		}
		else tracking = FALSE;
	}


	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && glutGetModifiers() == GLUT_ACTIVE_CTRL)
	{
		do_rotate = FALSE;
		tracking = FALSE;
		picked(x, y);	//If Ctrl is pressed send the x,y coordinates to the picking function
	}
}

/*Mouse-motion callback: this is used for rotation. It's based on the trackball scheme,
where every x,y coordinate of the mouse can be projected onto a sphere with specific
radius (for ease 1 is used), and use this principle to rotate in every posible direction
with only two axis. Taking two points on the sphere, we can get the angle & axis of rotation
by calculating the cross product of the formed vectors. mouse2sphere() finds the projection.*/

void motionCB(int x, int y)
{
	double length;
	
	if(do_rotate)
	{
	tracking = TRUE;

	mouse2sphere(x, y, b);
	
	vec_cross(a, b, axis);
	
	length = vec_module(axis);
	
	angle = length*180.0/M_PI;    //Refresh is at a high framerate, so sin(angle)=angle can be assumed 

    vec_normal(axis);
	
	vec_copy(b, a);
	}
	
	
}

void keybCB(unsigned char key, int x, int y)
{

	float w = glutGet(GLUT_WINDOW_WIDTH);
    float h = glutGet(GLUT_WINDOW_HEIGHT);

	if(key == 's' || key == 'S') do_animate = 1;
	if(key == 'd' || key == 'D') do_animate = -1;
	
	if((key == 'o' || key == 'O') && projection == ORTHO) 
	{
		projection = PERSPECTIVE;
		reshapeCB(w, h);	//We call reshape so the projection is refreshed
	}
	else if((key == 'o' || key == 'O') && projection == PERSPECTIVE)
	{
		projection = ORTHO;
		reshapeCB(w, h);
	}

}

void specialCB(int func, int x, int y)
{

	if(func == GLUT_KEY_UP) scale*=ZOOM_FACTOR;
	if(func == GLUT_KEY_DOWN) scale/=ZOOM_FACTOR;

}

void menuCB(int value)
{
	if(value == 9)		//Reinitialization
	{
		int i;
		for(i=0; i<16; i++)
			rotTemp[i]=0;
		for(i=0; i<16; i+=5)
			rotTemp[i]=1;

		for(i=0; i<7; i++)
		alpha_part[i] = FALSE;

		angle=0;
		scale=1;
		switch0=TRUE;
		switch1=TRUE;
		switch2=TRUE;
		animation=0;
		sub1 = 1;
		projection=ORTHO;
	}

	if(value == 0) exit(0);

	createMenu();
}

void config_menuCB(int value)
{
	sub1 = value;
}

void light_menuCB(int value)
{

	if(value == 3) switch0 =! switch0;	//light switches
	if(value == 4) switch1 =! switch1;
	if(value == 5) switch2 =! switch2;
	createMenu();

}


/*Made this a separate function so that the list can be refreshed for the ON/OFF switches*/
void createMenu()
{
	int sub_menu1 = glutCreateMenu(config_menuCB);
	glutAddMenuEntry("1 Large - 2 Small", 1);
	glutAddMenuEntry("2 Large - 1 Small", 2);
	int sub_menu2 = glutCreateMenu(light_menuCB);
	if(switch0) glutAddMenuEntry("Model ON", 3);
		else glutAddMenuEntry("Model OFF", 3);
	if(switch1) glutAddMenuEntry("Viewer ON", 4);
		else glutAddMenuEntry("Viewer OFF", 4);
	if(switch2) glutAddMenuEntry("Spotlight ON", 5);
		else glutAddMenuEntry("Spotlight OFF", 5);
    glutCreateMenu(menuCB); 
	glutAddSubMenu("Cogwheels Config", sub_menu1);
	glutAddSubMenu("Lighting", sub_menu2);
	glutAddMenuEntry("Reinitialize",9);
	glutAddMenuEntry("Exit", 0);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void timerCB(int value)
{
	if(value==1) animation+=0.5;	/*every timing (1 ms) rotate 0.5 degrees*/
	if(value==-1) animation-=0.5;
	
	if(animation >= 360.0) animation -= 360.0;	
	else if(animation <= -360.0) animation += 360.0;
	
	
	glutPostRedisplay();
	glutTimerFunc(1, timerCB, do_animate);
}

void main()
{
	int i;			
	
	//Initialize rotation temp matrix to identity
	for(i=0; i<16; i++)
		rotTemp[i]=0;
	for(i=0; i<16; i+=5)
		rotTemp[i]=1;
	
	//Initialize state of transparency for all objects (FALSE being opaque)
	for(i=0; i<7; i++)
		alpha_part[i] = FALSE;
		
	if(ReadOffFile("fixer.off",&Fixer)!=1)
	{
		printf("Problem reading off file!\n");
		return;
	}
	
	if(ReadOffFile("cw_small.off",&CW_Small)!=1)
	{
		printf("Problem reading off file!\n");
		return;
	}

	if(ReadOffFile("cw_large.off",&CW_Large)!=1)
	{
		printf("Problem reading off file!\n");
		return;
	}
	findFaceNormal(&Fixer);
	findFaceNormal(&CW_Small);
	findFaceNormal(&CW_Large);
	findVertexNormal(&Fixer);
	findVertexNormal(&CW_Small);
	findVertexNormal(&CW_Large);
	
	glutInitWindowSize(600, 600);
    glutInitWindowPosition(200,50);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	 
	int window_id = glutCreateWindow("Final Project - Elias Cohenca");

	createMenu();
	
	glutReshapeFunc(reshapeCB);
    glutDisplayFunc(drawingCB);
	glutSpecialFunc(specialCB);
	glutKeyboardFunc(keybCB);
	glutMouseFunc(mouseCB);
	glutMotionFunc(motionCB);

	glutTimerFunc(1, timerCB, do_animate);
	glutMainLoop();


}

