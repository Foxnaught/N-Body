#include <iostream>
#include <vector>
#include <memory>
#include <math.h>
#include <stdint.h>
#include "SDL/include/SDL.h"

using namespace std;

//Gravitational constant, a mass of 1 is equivalent to 1kg
//double G = 6.67408E-10f;
//double unitMass = 1.5*pow(10, 9);
//Toy constant, a mass of 1 is equivalent to asteroid size  ~1.5 billion kg
double G = 1;
double unitMass = 1;


vector<SDL_Point> getCirclePoints(int xOff, int yOff, int numPoints, double radius)
{
	vector<SDL_Point> circleCoords;
	double deltaPhi = 2*3.1415926/numPoints;
	for (int i=0; i<numPoints+1; i++)
	{
		double tempX = radius * cos(deltaPhi * i);
		double tempY = radius * sin(deltaPhi * i);
		SDL_Point tempCoord;
		tempCoord.x = tempX + xOff;
		tempCoord.y = tempY + yOff;
		
		circleCoords.push_back(tempCoord);
	}
	
	return circleCoords;
}

struct nbody
{
	double x;
	double y;
	double velX;
	double velY;
	double radius;
	uint32_t mass;
	bool staticBody;
};

nbody getNewNBody(int newX, int newY, double dX, double dY, int unitMasses, bool staticFlag)
{
	nbody newNBody;
	newNBody.x = newX;
	newNBody.y = newY;
	newNBody.velX = dX;
	newNBody.velY = dY;
	//By basing the mass and radius on a standard unit we can scale to large and small by changing the unit mass
	newNBody.mass = unitMass*unitMasses;
	newNBody.radius = 1.5;
	if (unitMasses > 1)
		newNBody.radius *= cbrt(unitMasses);

	newNBody.staticBody = staticFlag;

	return newNBody;
}

void placeRandomField(int massCount, double velocity, double radius, SDL_Window* renderWindow, vector<nbody>* nbodyList)
{
	int width;
	int height;
	SDL_GetWindowSize(renderWindow, &width, &height);
	for (int i=0; i<massCount; i++)
	{
		double dist = (double)rand()/RAND_MAX*radius;
		double placeAngle = (double)rand()/RAND_MAX*2*3.1415926;
		double x = cos(placeAngle) * dist + (double)width/2;
		double y = sin(placeAngle) * dist + (double)height/2;
		double angle = (double)rand()/RAND_MAX*2*3.1415926;
		double newVelX = cos(angle)*velocity;
		double newVelY = sin(angle)*velocity;
		nbody newBody = getNewNBody((int) x, (int) y, newVelX, newVelY, 1, false);
		nbodyList->push_back(newBody);
	}
}

void makeAccDisk(int massCount, double radius, double centerMass, SDL_Window* renderWindow, vector<nbody>* nbodyList)
{
	nbodyList->clear();
	int width;
	int height;
	SDL_GetWindowSize(renderWindow, &width, &height);
	nbody newBody = getNewNBody((double)width/2, (double)height/2, 0, 0, centerMass, true);
	nbodyList->push_back(newBody);
		
	for (int i=0;i<massCount;i++)
	{
		double newDist = (double)rand()/RAND_MAX*radius+50;
		double newAngle = (double)rand()/RAND_MAX*2*3.1415926;
		double dX = cos(newAngle)*newDist;
		double dY = sin(newAngle)*newDist;
		double totalVel = sqrt((centerMass*G)/newDist);
		double tanAngle = atan2(-dY, dX);
		double newVelX = sin(tanAngle)*totalVel;
		double newVelY = cos(tanAngle)*totalVel;
		nbody newBody = getNewNBody(dX + (double)width/2, dY + (double)height/2, newVelX, newVelY, 1, false);
		nbodyList->push_back(newBody);
	}	
}

void printTotalMomentum(vector<nbody>* nbodyList);

void updateBodies(vector<nbody>* nbodyList)
{
	//We will use a fixed timestep to create consistent results
	//The smaller the time step the more accurate the simulation will become but the slower it will run
	double timeStep = .1;
	for (int i=0; i<nbodyList->size(); i++)
	{
		nbody* curBody = &nbodyList->at(i);
		
		//Go through all other bodies and add their gravity effects to this body's velocity
		for (int t=nbodyList->size()-1; t >= 0; t--)
		{
			if (i != t)
			{
				nbody target = nbodyList->at(t);
				double distX = target.x - curBody->x;
				double distY = target.y - curBody->y;
				double totalDist = sqrt(distX*distX + distY*distY);
				
				//If one body is within another, merge them.
				bool withinRange = totalDist < target.radius || totalDist < curBody->radius;
				if ((withinRange && (curBody->mass >= target.mass || curBody->staticBody)) && !target.staticBody)
				{
					//Ideal elastic collision
					curBody->velX = (curBody->mass*curBody->velX + target.mass*target.velX)/(curBody->mass+target.mass);
					curBody->velY = (curBody->mass*curBody->velY + target.mass*target.velY)/(curBody->mass+target.mass);
					curBody->mass += target.mass;
					curBody->radius = cbrt(target.radius*target.radius*target.radius + curBody->radius*curBody->radius*curBody->radius);

					nbodyList->erase(nbodyList->begin()+t);
					//printTotalMomentum(nbodyList);
				}
				else
				{
					double accel = nbodyList->at(t).mass*G/(totalDist*totalDist);
					double accX = accel * distX/totalDist;
					double accY = accel * distY/totalDist;
					curBody->velX += accX*timeStep;
					curBody->velY += accY*timeStep;
				}
			}	
		}

		if (curBody->staticBody)
		{
			curBody->velX = 0;
			curBody->velY = 0;
		}
		
		//Add velocity to position
		curBody->x += curBody->velX*timeStep;
		curBody->y += curBody->velY*timeStep;
	}
}

void printTotalMomentum(vector<nbody>* nbodyList)
{
	double momentumX = 0.00, momentumY = 0.00;
	for (int i=0;i<nbodyList->size();i++)
	{
		nbody m = nbodyList->at(i);
		momentumX += m.mass*m.velX;
		momentumY += m.mass*m.velY;
	}

	cout << momentumX << endl;
	cout << momentumY << endl;
	cout << "----" << endl;
}

int main(int argc, char** argv)
{
	bool running = true;
	SDL_Event event;
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window *mainWin = SDL_CreateWindow("NBODY SIM", 100, 100, 1000, 720, SDL_WINDOW_SHOWN);
	SDL_Renderer *ren = SDL_CreateRenderer(mainWin, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	vector<nbody> nbodyList;

	//Keep track if we have been holding a keyboard or mouse button is being held down
	bool buttonFlag = false;
	//It can be hard to see multiple planets in orbit so the root scale flag will set the offset from the center of screen at root scale
	bool rootScale = false;
	bool leftClick = false;
	//Flags if we clicked and are setting up a new body but have not released it
	bool placingBody = false;
	//Flag if the body to be placed is to be static (0 velocity at all times)
	bool staticBody = false;
	//The center of the new body being placed
	int newX = 0;
	int newY = 0;
	int dX = 0;
	int dY = 0;
	double cameraOffsetX = 0;
	double cameraOffsetY = 0;
	while (running)
	{
		SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(ren);
		int width;
		int height;
		SDL_GetWindowSize(mainWin, &width, &height);
		double centerX = cameraOffsetX + (double)width/2;
		double centerY = cameraOffsetY + (double)height/2;

		//Apply gravitational acceleration between all bodies
		//Combine bodies that have moved too close to one another (perfectly elastic collision)
		updateBodies(&nbodyList);

		for (int i=0; i<nbodyList.size(); i++)
		{
			nbody curBody = nbodyList.at(i);
			vector<SDL_Point> circleCoords;
			//Get the points representing the circle of the body and render it
			//If we are in root scale then calculate the offset from the center and take the sqrt
			//This lets us seem things that have flown far beyond the screen
			if (rootScale)
			{
				//Get the offset from the center of the screen
				double xOff = curBody.x - centerX;
				double yOff = curBody.y - centerY;
				//Get the distance from the center of the screen
				double dist = sqrt(xOff*xOff + yOff*yOff);
				//Get the root distance and split it into its components
				double rootDist = sqrt(dist);
				double rootScaleX = 0, rootScaleY = 0;
				if (rootDist > 0)
				{
					rootScaleX = rootDist*xOff/dist;
					rootScaleY = rootDist*yOff/dist;
				}

				circleCoords = getCirclePoints((double)width/2 + rootScaleX, (double)height/2 + rootScaleY, 20, curBody.radius);
			}
			else
			{
				circleCoords = getCirclePoints(-cameraOffsetX + curBody.x, -cameraOffsetY + curBody.y, 20, curBody.radius);
				vector<SDL_Point> velVec;
				SDL_Point start, end;
				start.x = -cameraOffsetX + curBody.x;
				start.y = -cameraOffsetY + curBody.y;
				end.x = -cameraOffsetX + curBody.x+curBody.velX*1;
				end.y = -cameraOffsetY + curBody.y+curBody.velY*1;
				velVec.push_back(start);
				velVec.push_back(end);
				SDL_SetRenderDrawColor(ren, 0xFF, 0x00, 0x00, 0xFF);
				SDL_RenderDrawLines(ren, velVec.data(), velVec.size());
			}
			
			SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);
			SDL_RenderDrawLines(ren, circleCoords.data(), circleCoords.size());
		}


		if (rootScale)
		{
			double windowX = sqrt((double)width/2);
			double windowY = sqrt((double)height/2);
			vector<SDL_Point> logWindowPoints;
			SDL_Point topLeft, topRight, bottomLeft, bottomRight;
			topLeft.x = -windowX + (double)width/2;
			topLeft.y = -windowY + (double)height/2;
			topRight.x = windowX + (double)width/2;
			topRight.y = -windowY + (double)height/2;
			bottomLeft.x = -windowX + (double)width/2;
			bottomLeft.y = windowY + (double)height/2;
			bottomRight.x = windowX + (double)width/2;
			bottomRight.y = windowY + (double)height/2;
			logWindowPoints.push_back(topLeft);
			logWindowPoints.push_back(topRight);
			logWindowPoints.push_back(bottomRight);
			logWindowPoints.push_back(bottomLeft);
			logWindowPoints.push_back(topLeft);

			SDL_RenderDrawLines(ren, logWindowPoints.data(), logWindowPoints.size());
		}

		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT)
			running = false;
		
		const Uint8* keystate = SDL_GetKeyboardState(NULL);
		int mouseX;
		int mouseY;
		double moveDiff = 3;
		Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
		if (keystate[SDL_SCANCODE_C])
		{
			nbodyList.clear();
		}
		else if (keystate[SDL_SCANCODE_R])
		{
			cameraOffsetX = 0;
			cameraOffsetY = 0;
		}
		else if (keystate[SDL_SCANCODE_LEFT])
		{
			cameraOffsetX -= moveDiff;
		}
		else if (keystate[SDL_SCANCODE_RIGHT])
		{
			cameraOffsetX += moveDiff;
		}
		else if (keystate[SDL_SCANCODE_UP])
		{
			cameraOffsetY -= moveDiff;
		}
		else if (keystate[SDL_SCANCODE_DOWN])
		{
			cameraOffsetY += moveDiff;
		}
		else if (keystate[SDL_SCANCODE_A] && !buttonFlag)
		{
			makeAccDisk(2000, height, 200000, mainWin, &nbodyList);
			buttonFlag = true;
		}
		else if (keystate[SDL_SCANCODE_L])
		{
			//Prevent multiple firings from one press
			if (!buttonFlag)
				rootScale = !rootScale;
			buttonFlag = true;
		}
		else if (keystate[SDL_SCANCODE_P] && !buttonFlag)
		{
			placeRandomField(30, .5, 5*height, mainWin, &nbodyList);
			buttonFlag = true;
		}
		else if (mouseState && !placingBody)
		{
			placingBody = true;
			leftClick = true;
			if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
			{
				staticBody = true;
				leftClick = false;
			}
			
			newX = mouseX;
			newY = mouseY;
		}
		else if (mouseState == 0 && placingBody)
		{
			nbody newBody = getNewNBody(-cameraOffsetX + newX, -cameraOffsetY + newY, (double)(newX-dX)/20, (double)(newY-dY)/20, 1, staticBody);
			nbodyList.push_back(newBody);

			leftClick = false;
			placingBody = false;
			staticBody = false;
		}
		else
		{
			//buttonFlag is used to prevent a toggle-able state from firing more than once on a single press
			buttonFlag = false;
		}

		if (placingBody && leftClick)
		{
			vector<SDL_Point> circleCoords = getCirclePoints(-cameraOffsetX + newX, -cameraOffsetY + newY, 20, 3);
			SDL_SetRenderDrawColor(ren, 0xFF, 0x00, 0x00, 0xFF);
			SDL_RenderDrawLines(ren, circleCoords.data(), circleCoords.size());
			SDL_GetMouseState(&dX, &dY);
			SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);
			SDL_RenderDrawLine(ren, newX, newY, dX, dY);
		}


		//Update screen
		SDL_RenderPresent(ren);
	}

	SDL_DestroyWindow(mainWin);
	return 0;
}


