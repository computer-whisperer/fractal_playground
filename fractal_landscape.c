#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>


typedef int BOOL;
#define TRUE 1
#define FALSE 0

static GLfloat g_nearPlane = 0.0001;
static GLfloat g_farPlane = 1000;

static int g_Width = 600;                          // Initial window width
static int g_Height = 600;                         // Initial window height

//static float g_lightPos[4] = {10, 10, -100, 1 };  // Position of light

static struct timeval last_idle_time;

static float cameraYaw;
static float cameraPitch;
static float cameraX;
static float cameraY;
static float cameraZ;

static float yawVel;
static float pitchVel;
static float xVel;
static float yVel;
static float zVel;

static float velSet;
static float accelSet;

struct FaceSet {
	struct Face * head;
} faceSet;

struct Point {
	float x;
	float y;
	float z;
	int seed;
};

struct Edge {
	struct Point * points[2];
	struct Edge * children[2];
};

struct Face {
	struct Face * prevInOrder;
	struct Face * nextInOrder;
	struct Face * parent;
	struct Face * children[4];
	struct Edge * edges[3];
	int generation;
};


void promoteEdge(struct Edge * edge) {
	if (edge->children[0] != NULL)
		return;
		
	struct Point * a = edge->points[0];
	struct Point * b = edge->points[1];
	
	float midX = (a->x + b->x)/2;
	float midY = (a->y + b->y)/2;
	float midZ = (a->z + b->z)/2;
	
	float len = sqrt(pow(a->x - b->x, 2) + pow(a->y - b->y, 2) + pow(a->z - b->z, 2));
	
	int seed = (a->seed + b->seed)%10;
	
	float disp = len * ((float)seed - 5)/60;
	
	struct Point * newPoint = malloc(sizeof(struct Point));
	newPoint->x = midX;
	newPoint->y = midY + disp;
	newPoint->z = midZ;
	newPoint->seed = seed;
	
	struct Edge * edge0 = malloc(sizeof(struct Edge));
	edge0->points[0] = edge->points[0];
	edge0->points[1] = newPoint;
	edge0->children[0] = NULL;
	edge0->children[1] = NULL;
	
	struct Edge * edge1 = malloc(sizeof(struct Edge));
	edge1->points[0] = newPoint;
	edge1->points[1] = edge->points[1];
	edge1->children[0] = NULL;
	edge1->children[1] = NULL;
	
	edge->children[0] = edge0;
	edge->children[1] = edge1;
}

struct Face * promoteFace(struct Face * face) {
	promoteEdge(face->edges[0]);
	promoteEdge(face->edges[1]);
	promoteEdge(face->edges[2]);
	
	struct Point * mid0 = face->edges[0]->children[0]->points[1];
	struct Point * mid1 = face->edges[1]->children[0]->points[1];
	struct Point * mid2 = face->edges[2]->children[0]->points[1];
	
	struct Edge * edge0 = malloc(sizeof(struct Edge));
	edge0->points[0] = mid2;
	edge0->points[1] = mid1;
	edge0->children[0] = NULL;
	
	struct Edge * edge1 = malloc(sizeof(struct Edge));
	edge1->points[0] = mid1;
	edge1->points[1] = mid0;
	edge1->children[0] = NULL;
	
	struct Edge * edge2 = malloc(sizeof(struct Edge));
	edge2->points[0] = mid0;
	edge2->points[1] = mid2;
	edge2->children[0] = NULL;
	
	struct Face * face0 = malloc(sizeof(struct Face));
	face0->parent = face;
	face0->generation = face->generation+1;
	face0->children[0] = NULL;
	face0->edges[0] = edge0;
	face0->edges[1] = face->edges[1]->children[1];
	face0->edges[2] = face->edges[2]->children[0];
	
	struct Face * face1 = malloc(sizeof(struct Face));
	face1->parent = face;
	face1->generation = face->generation+1;
	face1->children[0] = NULL;
	face1->edges[0] = edge2;
	face1->edges[1] = face->edges[2]->children[1];
	face1->edges[2] = face->edges[0]->children[0];
	
	struct Face * face2 = malloc(sizeof(struct Face));
	face2->parent = face;
	face2->generation = face->generation+1;
	face2->children[0] = NULL;
	face2->edges[0] = edge1;
	face2->edges[1] = face->edges[0]->children[1];
	face2->edges[2] = face->edges[1]->children[0];
	
	struct Face * face3 = malloc(sizeof(struct Face));
	face3->parent = face;
	face3->generation = face->generation+1;
	face3->children[0] = NULL;
	face3->edges[0] = edge0;
	face3->edges[1] = edge1;
	face3->edges[2] = edge2;
	
	face0->prevInOrder = face->prevInOrder;
	face0->nextInOrder = face1;
	face1->prevInOrder = face0;
	face1->nextInOrder = face2;
	face2->prevInOrder = face1;
	face2->nextInOrder = face3;
	face3->prevInOrder = face2;
	face3->nextInOrder = face->nextInOrder;
	
	if (face->prevInOrder != NULL)
		face->prevInOrder->nextInOrder = face0;
	if (face->nextInOrder != NULL)
		face->nextInOrder->prevInOrder = face3;
	
	face->prevInOrder = NULL;
	face->nextInOrder = NULL;
	face->children[0] = face0;
	face->children[1] = face1;
	face->children[2] = face2;
	face->children[3] = face3;
	
	return face0;
}

void deleteEdge(struct Edge * edge){
  if (edge->children[0] != NULL)
    deleteEdge(edge->children[0]);
  if (edge->children[1] != NULL)
    deleteEdge(edge->children[1]);
  free(edge);
}

struct Face * demoteFace(struct Face * face) {
  struct Face * parentFace = face->parent;
  
  for (int i = 0; i < 4; i++) {
    if (parentFace->children[i]->children[0] != NULL) {
      demoteFace(parentFace->children[i]->children[0]);
    }
  }

  parentFace->nextInOrder = parentFace->children[0]->nextInOrder;
  if (parentFace->nextInOrder != NULL)
    parentFace->nextInOrder->prevInOrder = parentFace;
    
  parentFace->prevInOrder = parentFace->children[0]->prevInOrder;
  if (parentFace->prevInOrder != NULL)
    parentFace->prevInOrder->nextInOrder = parentFace;
  
  for (int i = 1; i < 4; i++) {
    struct Face * child = parentFace->children[i];
    if (child->nextInOrder != NULL)
      child->nextInOrder->prevInOrder = child->prevInOrder;
    if (child->prevInOrder != NULL)
      child->prevInOrder->nextInOrder = child->nextInOrder;
  }
  //deleteEdge(parentFace->children[0]->edges[0]);
  //deleteEdge(parentFace->children[1]->edges[0]);
  //deleteEdge(parentFace->children[2]->edges[0]);
  free(parentFace->children[0]);
  free(parentFace->children[1]);
  free(parentFace->children[2]);
  free(parentFace->children[3]);
  parentFace->children[0] = NULL;
  parentFace->children[1] = NULL;
  parentFace->children[2] = NULL;
  parentFace->children[3] = NULL;
  
  return parentFace;
}


float faceDisplacement(struct Face * face, struct Point * point) {
	struct Point * a = face->edges[0]->points[0];
	struct Point * b = face->edges[1]->points[0];
	struct Point * c = face->edges[2]->points[0];
	float midX = (a->x + b->x + c->x)/3;
	float midY = (a->y + b->y + c->y)/3;
	float midZ = (a->z + b->z + c->z)/3;
	
	return sqrt(pow(midX - point->x, 2) + pow(midY - point->y, 2) + pow(midZ - point->z, 2));
}

void renderPoint(struct Point * point) {
	//glColor3f(0, 0, point->y*2+0.5);
	glVertex3f(point->x, point->y, point->z);
}

void renderWorld(GLenum mode) {
  glBegin(mode);
  struct Face * currFace = faceSet.head;
  int facesRendered = 0;
  while (currFace != NULL) {
    if (mode == GL_TRIANGLES) {
      renderPoint(currFace->edges[0]->points[0]);
      renderPoint(currFace->edges[1]->points[0]);
      renderPoint(currFace->edges[2]->points[0]);
      if (currFace->edges[0]->children[0] != NULL) {
        renderPoint(currFace->edges[0]->points[0]);
        renderPoint(currFace->edges[0]->children[0]->points[1]);
        renderPoint(currFace->edges[0]->points[1]);      
      }
      if (currFace->edges[1]->children[0] != NULL) {
        renderPoint(currFace->edges[1]->points[0]);
        renderPoint(currFace->edges[1]->children[0]->points[1]);
        renderPoint(currFace->edges[1]->points[1]);      
      }
      if (currFace->edges[2]->children[0] != NULL) {
        renderPoint(currFace->edges[2]->points[0]);
        renderPoint(currFace->edges[2]->children[0]->points[1]);
        renderPoint(currFace->edges[2]->points[1]);      
      }
    }
	   
    if (mode == GL_LINES) {
		   renderPoint(currFace->edges[0]->points[0]);
		   renderPoint(currFace->edges[0]->points[1]);
		   renderPoint(currFace->edges[1]->points[0]);
		   renderPoint(currFace->edges[1]->points[1]);
		   renderPoint(currFace->edges[2]->points[0]);
		   renderPoint(currFace->edges[2]->points[1]);
    }
    currFace = currFace->nextInOrder;
    facesRendered++;
  }
  glEnd();
  printf("%i faces rendered\n", facesRendered);
}

void display(void)
{
   // Clear frame buffer and depth buffer
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glPushMatrix();

   // Set up viewing transformation, looking down -Z axis
   glLoadIdentity();
   gluLookAt(0, 0, 0, // eye position
			 1, 0, 0,  // focus position
			 0, 1, 0); // up vector

   // Set up the stationary light
   //glLightfv(GL_LIGHT0, GL_POSITION, g_lightPos);


   // Render the scene
   float colorBlue[4]       = { 0.0, 0.2, 1.0, 1.0 };
   float colorNone[4]       = { 0.0, 0.0, 0.0, 0.0 };
   glMaterialfv(GL_FRONT, GL_DIFFUSE, colorBlue);
   glMaterialfv(GL_FRONT, GL_SPECULAR, colorNone);
   glColor4fv(colorBlue);
   
   glRotatef(cameraPitch, 0, 0, 1);
   glRotatef(cameraYaw, 0, 1, 0);
   glTranslatef(-cameraX, -cameraY, -cameraZ);

   glColor3f(0, 0, 0);
   renderWorld(GL_TRIANGLES);
   glTranslatef(0, 0.0001, 0);
   glColor3f(0, 0, 100);
   renderWorld(GL_LINES);
   
   glPopMatrix();

   // Make sure changes appear onscreen
   glutSwapBuffers();
}

void reshape(GLint width, GLint height)
{
   g_Width = width;
   g_Height = height;

   glViewport(0, 0, g_Width, g_Height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(65.0, (float)g_Width / g_Height, g_nearPlane, g_farPlane);
   glMatrixMode(GL_MODELVIEW);
}

void InitGraphics(void)
{

   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);
   glShadeModel(GL_SMOOTH);
   //glEnable(GL_LIGHTING);
   //glEnable(GL_LIGHT0);

}

void update(void)
{
  float dt;

  // Figure out time elapsed since last call to idle function
  struct timeval time_now;
  gettimeofday(&time_now, NULL);
  dt = (float)(time_now.tv_sec  - last_idle_time.tv_sec) +
  1.0e-6*(time_now.tv_usec - last_idle_time.tv_usec);
  
  velSet += dt * accelSet;

  cameraPitch += dt * 30 * pitchVel;
  cameraYaw += dt * 30 * yawVel;
  cameraX += dt * pow(2, velSet) * xVel;
  cameraY += dt * pow(2, velSet) * yVel;
  cameraZ += dt * pow(2, velSet) * zVel;
  

  // Save time_now for next time
  last_idle_time = time_now;
  
  // Reassign depth
  struct Face * currFace = faceSet.head;
  struct Point point = {cameraX, cameraY, cameraZ, 0};
  while (currFace != NULL) {
	  float dist = fmax(0.00001, faceDisplacement(currFace, &point));
//    int idealLevel = (int)fmin(log(50/dist - 10), maxLevel);
    int idealLevel = fmax(2, log(200/dist)/log(2));
	  if (currFace->generation < idealLevel){
		  struct Face * newFace = promoteFace(currFace);
		  if (currFace == faceSet.head)
        faceSet.head = newFace;
      currFace = newFace;
	  }
    else if (currFace->generation > idealLevel + 1){
		  struct Face * newFace = demoteFace(currFace);
		  if (currFace == faceSet.head)
        faceSet.head = newFace;
      currFace = newFace;
	  }
    else {
      currFace = currFace->nextInOrder;
    }
  }

  // Force redraw
  glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{
  yawVel= 0;
  pitchVel = 0;
  xVel = 0;
  yVel = 0;
  zVel = 0;
  accelSet = 0;
  
  switch (key)
  {
  case 27:             // ESCAPE key
	  exit (0);
	  break;

  case 'w':
	  xVel = 1;
	  break;
  
  case 's':
	  xVel = -1;
	  break;
    
  case 'a':
	  zVel = -1;
	  break;
  
  case 'd':
	  zVel = 1;
	  break;

  case 'j':
	  yawVel = 1;
	  break;
    
  case 'l':
	  yawVel = -1;
	  break;

  case 'i':
	  pitchVel = 1;
	  break;
  
  case 'k':
	  pitchVel = -1;
	  break;
    
  case 'r':
    yVel = 1;
    break;
  
  case 'f':
    yVel = -1;
    break;
  
  case '[':
    accelSet = 1;
    break;
  
  case ']':
    accelSet = -1;
    break;
  }
}


int main(int argc, char** argv)
{
	
  cameraYaw = 0;
  cameraPitch = 0;
  cameraX = 0;
  cameraY = 0;
  cameraZ = 0;
  velSet = 0;
	
  // Create starting face
  struct Point * pointA = malloc(sizeof(struct Point));
  pointA->x = -2;
  pointA->y = 0;
  pointA->z = -1;
  pointA->seed = 1;
  
  struct Point * pointB = malloc(sizeof(struct Point));
  pointB->x = 2;
  pointB->y = 0;
  pointB->z = -1;
  pointB->seed = 1;
  
  struct Point * pointC = malloc(sizeof(struct Point));
  pointC->x = 0;
  pointC->y = 0;
  pointC->z = 2;
  pointC->seed = 1;
  
  struct Edge * edgeA = malloc(sizeof(struct Edge));
  edgeA->points[0] = pointA;
  edgeA->points[1] = pointB;
  edgeA->children[0] = NULL;
  
  struct Edge * edgeB = malloc(sizeof(struct Edge));
  edgeB->points[0] = pointB;
  edgeB->points[1] = pointC;
  edgeB->children[0] = NULL;
  
  struct Edge * edgeC = malloc(sizeof(struct Edge));
  edgeC->points[0] = pointC;
  edgeC->points[1] = pointA;
  edgeC->children[0] = NULL;
  
  struct Face * startFace = malloc(sizeof(struct Face));
  startFace->nextInOrder = NULL;
  startFace->prevInOrder = NULL;
  startFace->generation = 0;
  startFace->parent = NULL;
  startFace->children[0] = NULL;
  startFace->edges[0] = edgeA;
  startFace->edges[1] = edgeB;
  startFace->edges[2] = edgeC;
  
  faceSet.head = startFace;
  
  // Uniform densify
  /*
  for (int i = 0; i < 9; i++){
	  struct Face * currFace = faceSet.head;
	  while (currFace != NULL) {
		  if (currFace->generation == i){
			  struct Face * newFace = promoteFace(currFace);
			  if (currFace == faceSet.head)
				faceSet.head = newFace;
			  currFace = newFace;
		  }
		  else {
		      currFace = currFace->nextInOrder;
	      }
	  }
  }*/
  
  // Scaling densify
  /*
  struct Face * currFace = faceSet.head;
  int maxLevel = 11;
  struct Point point = {0, 0, 0, 0};
  while (currFace != NULL) {
	  float dist = faceDisplacement(currFace, &point);
	  if (currFace->generation < maxLevel-(dist*8)){
		  struct Face * newFace = promoteFace(currFace);
		  if (currFace == faceSet.head)
        faceSet.head = newFace;
      currFace = newFace;
	  }
    else {
      currFace = currFace->nextInOrder;
    }
  }
  */
  
  // GLUT Window Initialization:
  glutInit (&argc, argv);
  glutInitWindowSize (g_Width, g_Height);
  glutInitDisplayMode ( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow ("Fractal Landscape");

  // Initialize OpenGL graphics state
  InitGraphics();

  // Register callbacks
  glutDisplayFunc (display);
  glutReshapeFunc (reshape);
  glutKeyboardFunc (keyboard);
  glutIdleFunc (update);

  // Get the initial time, for use by animation
  gettimeofday (&last_idle_time, NULL);

  // Turn the flow of control over to GLUT
  glutMainLoop ();
  return 0;
}

 

