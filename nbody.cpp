#include <iostream>
#include <vector>
#include <memory>
#include <math.h>
#include <SDL2/SDL.h>


std::vector<SDL_Point>* getCirclePoints(int xOff, int yOff, int numPoints, double radius)
{
	std::vector<SDL_Point>* circleCoords = new std::vector<SDL_Point>();
	double deltaPhi = 2*3.1415926/numPoints;
	for (int i=0; i<numPoints+1; i++)
	{
		double tempX = radius * cos(deltaPhi * i);
		double tempY = radius * sin(deltaPhi * i);
		SDL_Point tempCoord;
		tempCoord.x = tempX + xOff;
		tempCoord.y = tempY + yOff;
		
		circleCoords->push_back(tempCoord);
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
	double mass;
	bool staticBody;
};

nbody getNewNBody(int newX, int newY, double dX, double dY)
{
	nbody* newNBody = new nbody();
	newNBody->x = newX;
	newNBody->y = newY;
	newNBody->velX = dX;
	newNBody->velY = dY;
	newNBody->mass = 1;
	newNBody->radius = 1.5;

	nbody retBody = *newNBody;
	delete newNBody;
	return retBody;
}

int main()
{
	bool running = true;
	SDL_Event event;
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window *mainWin = SDL_CreateWindow("Hello World!", 100, 100, 1000, 720, SDL_WINDOW_SHOWN);
	SDL_Renderer *ren = SDL_CreateRenderer(mainWin, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	std::vector<nbody> nbodyList;

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
	//We will use a fixed timestep to create consistents results
	//The smaller the time step the more accurate the simulation will become but the slower it will run
	double timeStep = .2;
	while (running)
	{
		SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(ren);
		int width;
		int height;
		SDL_GetWindowSize(mainWin, &width, &height);
		
		for (int i=0; i<nbodyList.size(); i++)
		{
			nbody* curBody = &nbodyList.at(i);
			
			//Go through all other bodies and add their gravity effects to this body's velocity
			for (int t=nbodyList.size()-1; t >= 0; t--)
			{
				if (i != t)
				{
					nbody target = nbodyList.at(t);
					//This should be the gravitational constant but since this is a toy physics environment...
					double G = 1;
					double distX = target.x - curBody->x;
					double distY = target.y - curBody->y;
					double totalDist = sqrt(distX*distX + distY*distY);
					
					//If one body is within another, merge them.
					bool withinRange = totalDist < target.radius || totalDist < curBody->radius;
					if ((withinRange && (curBody->mass >= target.mass || curBody->staticBody)) && !target.staticBody)
					{
						//Ideal inelastic collision
						curBody->velX = (curBody->mass*curBody->velX + target.mass*target.velX)/(curBody->mass+target.mass);
						curBody->velY = (curBody->mass*curBody->velY + target.mass*target.velY)/(curBody->mass+target.mass);
						curBody->mass += target.mass;
						curBody->radius = cbrt(target.radius*target.radius*target.radius + curBody->radius*curBody->radius*curBody->radius);
						
						nbodyList.erase(nbodyList.begin()+t);
					}
					else
					{
						double accel = nbodyList.at(t).mass*G/(totalDist*totalDist);
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
			
			std::vector<SDL_Point>* circleCoords;
			//Get the points representing the circle of the body and render it
			//If we are in root scale then calculate the offset from the center and take the sqrt
			//This lets us seem things that have flown far beyond the screen
			if (rootScale)
			{
				//Get the offset from the center of the screen
				double xOff = curBody->x - (double)width/2;
				double yOff = curBody->y - (double)height/2;
				//In order to do log scale around the center of the screen we have to take the log of the absolute value
				//And flip it on the axis if it is negative
				int xNeg = 1;
				if (xOff < 0)
					xNeg = -1;
				int yNeg = 1;
				if (yOff < 0)
					yNeg = -1;
				
				double rootScaleX = xNeg*sqrt(xNeg*xOff);
				double rootScaleY = yNeg*sqrt(yNeg*yOff);
				circleCoords = getCirclePoints((double)width/2 + rootScaleX, (double)height/2 + rootScaleY, 20, curBody->radius);
			}
			else
			{
				circleCoords = getCirclePoints(curBody->x, curBody->y, 20, curBody->radius);
			}
			
			SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);
			SDL_RenderDrawLines(ren, circleCoords->data(), circleCoords->size());
			delete circleCoords;
		}

		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT)
			running = false;
		
		const Uint8* keystate = SDL_GetKeyboardState(NULL);
		int mouseX;
		int mouseY;
		Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
		if (keystate[SDL_SCANCODE_C])
		{
			nbodyList.clear();
		}
		else if (keystate[SDL_SCANCODE_A])
		{
			nbodyList.clear();
			nbody newBody = getNewNBody((double)width/2, (double)height/2, 0, 0);
			newBody.staticBody = true;
			newBody.mass = 1000;
			newBody.radius = cbrt(1000);
			nbodyList.push_back(newBody);
			
			for (int i=0;i<1000;i++)
			{
				double newX = rand() % width - (double)width/2;
				double newY = rand() % height - (double)height/2;
				double angle = atan2(-newY, newX);
				double totalVel = sqrt(1000)/sqrt(sqrt(newX*newX+newY*newY));
				double newVelX = sin(angle)*totalVel;
				double newVelY = cos(angle)*totalVel;
				nbody newBody = getNewNBody(newX + (double)width/2, newY + (double)height/2, newVelX, newVelY);
				nbodyList.push_back(newBody);
			}
			
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
			int width;
			int height;
			SDL_GetWindowSize(mainWin, &width, &height);
			for (int i=0; i<30; i++)
			{
				int x = rand() % width;
				int y = rand() % height;
				int rX = rand() % 30;
				int rY = rand() % 30;
				if (rand() % 2 == 0)
					rX = -rX;
				if (rand() % 2 == 0)
					rY = -rY;

				rX = 0;
				rY = 0;
				
				nbody newBody = getNewNBody(x, y, (double)rX/20, (double)rY/20);
				nbodyList.push_back(newBody);
			}
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
			nbody newBody = getNewNBody(newX, newY, (double)(newX-dX)/20, (double)(newY-dY)/20);
			newBody.staticBody = staticBody;
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
			std::vector<SDL_Point>* circleCoords = getCirclePoints(newX, newY, 20, 3);
			SDL_SetRenderDrawColor(ren, 0xFF, 0x00, 0x00, 0xFF);
			SDL_RenderDrawLines(ren, circleCoords->data(), circleCoords->size());
			SDL_GetMouseState(&dX, &dY);
			SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);
			SDL_RenderDrawLine(ren, newX, newY, dX, dY);
			delete circleCoords;
		}


		//Update screen
		SDL_RenderPresent(ren);
	}

	SDL_DestroyWindow(mainWin);
	return 0;
}


