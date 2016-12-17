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

nbody* getNewNBody(int newX, int newY, int dX, int dY)
{
	nbody* newNBody = new nbody();
	newNBody->x = newX;
	newNBody->y = newY;
	newNBody->velX = (newX - dX)/20.0;
	newNBody->velY = (newY - dY)/20.0;
	newNBody->mass = 1;
	newNBody->radius = 1.5;

	nbody* retBody = newNBody;
	return retBody;
}

int main()
{
	bool running = true;
	SDL_Event event;
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window *mainWin = SDL_CreateWindow("Hello World!", 100, 100, 1280, 960, SDL_WINDOW_SHOWN);
	SDL_Renderer *ren = SDL_CreateRenderer(mainWin, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	std::vector<nbody> nbodyList;

	//Flags if we clicked and are setting up a new body but have not released it
	bool placingBody = false;
	//Flag if the body to be placed is to be static (0 velocity at all times)
	bool staticBody = false;
	//The center of the new body being placed
	int newX = 0;
	int newY = 0;
	int dX = 0;
	int dY = 0;
	while (running)
	{
		SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(ren);
		
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
					if (withinRange && curBody->mass >= target.mass && !target.staticBody)
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
						curBody->velX += accX;
						curBody->velY += accY;
					}
				}	
			}

			if (curBody->staticBody)
			{
				curBody->velX = 0;
				curBody->velY = 0;
			}
			
			//Add velocity to position
			curBody->x += curBody->velX;
			curBody->y += curBody->velY;
			
			//Get the points representing the circle of the body and render it
			std::vector<SDL_Point>* circleCoords = getCirclePoints(curBody->x, curBody->y, 20, curBody->radius);
			SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);
			SDL_RenderDrawLines(ren, circleCoords->data(), circleCoords->size());
			delete circleCoords;
		}

		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT)
			running = false;
		else if (event.type == SDL_KEYDOWN && (char)event.key.keysym.scancode == SDL_SCANCODE_P)
		{
			for (int i=0; i<30; i++)
			{
				int width;
				int height;
				SDL_GetWindowSize(mainWin, &width, &height);
				
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
				
				nbody newNBody = *getNewNBody(x, y, x+rX, y+rY);
				nbodyList.push_back(newNBody);
			}
		}
		else if (event.type == SDL_MOUSEBUTTONDOWN && !placingBody)
		{
			placingBody = true;
			if (event.button.button == SDL_BUTTON_RIGHT)
			{
				staticBody = true;
			}
			
			newX = event.button.x;
			newY = event.button.y;
		}
		else if (event.type == SDL_MOUSEBUTTONUP && placingBody)
		{
			nbody newNBody = *getNewNBody(newX, newY, dX, dY);
			newNBody.staticBody = staticBody;
			nbodyList.push_back(newNBody);

			staticBody = false;
			placingBody = false;
		}

		if (placingBody)
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


