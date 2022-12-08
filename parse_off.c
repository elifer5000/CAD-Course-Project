#include <stdio.h>

#define MAX_VERTICES 1000
#define MAX_FACES 2000
#define MAX_POINTS_PER_FACE 4
#define BUFSIZE 100

typedef double vect3D[3];
typedef int vecti[MAX_POINTS_PER_FACE];

typedef struct MODEL{
	int numberOfPoints;
	int numberOfFaces;
	vect3D points[MAX_VERTICES];
	vecti faces[MAX_FACES];
	int pointsPerFace[MAX_FACES];
} Model;

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

void ListOffModel(const Model* M)
{
	int numberOfPoints, numberOfFaces, i, j;

	numberOfPoints=M->numberOfPoints;
	numberOfFaces=M->numberOfFaces;
	printf("Number of vertices: %d\n",numberOfPoints);
	printf("Number of faces: %d\n",numberOfFaces);
	printf("The list of vertices:\n");
	for(i=0;i<numberOfPoints;i++){
		printf("Vertrx %d = (%lf,%lf,%lf)\n",i,M->points[i][0],M->points[i][1],M->points[i][2]);
	}
	printf("The list of faces:\n");
	for(i=0;i<numberOfFaces;i++){
		printf("Face %d has %d vertices, which are: ",i,M->pointsPerFace[i]);
		for(j=0;j<M->pointsPerFace[i];j++){
			printf("%d ",M->faces[i][j]);
		}
		printf("\n");
	}
}

void main()
{
	Model M;
	if(ReadOffFile("demo.off",&M)!=1){
		printf("Problem reading off file!\n");
		return;
	}
	ListOffModel(&M);
}