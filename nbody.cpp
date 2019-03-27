#include <iostream>
#include <vector>
#include <memory>
#include <math.h>
#include <stdint.h>
#include "SDL/include/SDL.h"
#include "SDL/include/SDL_ttf.h"
#include <CL/cl.hpp>
#include "windows.h"
#include <chrono>
#include <fstream>

using namespace std;

//Gravitational constant, a mass of 1 is equivalent to 1kg
//double G = 6.67408E-11f;
//double unitMass = 1.5*pow(10, 10);
//Toy constant, a mass of 1 is equivalent to asteroid size  ~15 billion kg
double G = 1;
double unitMass = 1;
double scale = 1.0;

class MenuItem
{
public:
	int x;
	int y;
	int width;
	int height;
	int borderSize = 0;
	bool hasDragBar = false; //Bar at the top of menu for moving it
	bool transparent = false;
	string text = "";
	vector<MenuItem*> children;
	SDL_Renderer* ren;
	TTF_Font* font;
	SDL_Surface* surface;
	SDL_Texture* texture;
	SDL_Color color = { 255, 255, 255 };
	SDL_Color background = {0, 0, 0};
	MenuItem(SDL_Renderer* argRen, int offsetX, int offsetY, int argWidth, int argHeight, int argBorderSize=0)
	{
		this->ren = argRen;
		this->x = offsetX;
		this->y = offsetY;
		this->width = argWidth;
		this->height = argHeight;
		this->font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 11);
		this->surface = TTF_RenderText_Solid(this->font, this->text.c_str(), this->color);
		this->texture = SDL_CreateTextureFromSurface(argRen, this->surface);
		this->borderSize = argBorderSize;
	}

	~MenuItem()
	{
		TTF_CloseFont(this->font);
	}

	void setDragBar(bool argDragBar)
	{
		this->hasDragBar = argDragBar;
	}

	void setText(string argText)
	{
		SDL_FreeSurface(this->surface);
		SDL_DestroyTexture(this->texture);
		this->text = argText;
		this->surface = TTF_RenderText_Solid(this->font, this->text.c_str(), this->color);
		this->texture = SDL_CreateTextureFromSurface(this->ren, this->surface);
	}

	void captureInput();

	void render(int globalX, int globalY, int mouseX, int mouseY)
	{
		if (!this->transparent)
		{
			SDL_SetRenderDrawColor(this->ren, 0, 0, 0, 255);
			SDL_Rect back;
			back.x = globalX;
			back.y = globalY;
			back.w = this->width;
			back.h = this->height;
			SDL_RenderFillRect(this->ren, &back);
		}

		if (this->text != "")
		{
			int textureWidth, textureHeight;
			SDL_QueryTexture(this->texture, nullptr, nullptr, &textureWidth, &textureHeight);

			SDL_Rect Message_rect; //create a rect
			Message_rect.x = globalX + this->width/2 - textureWidth/2;  //controls the rect's x coordinate 
			Message_rect.y = globalY + this->height/2 - textureHeight/2; // controls the rect's y coordinte
			Message_rect.w = textureWidth; // controls the width of the rect
			Message_rect.h = textureHeight; // controls the height of the rect
			SDL_RenderCopy(ren, this->texture, NULL, &Message_rect);
		}

		if (this->borderSize)
		{
			SDL_Rect border;
			border.x = globalX;
			border.y = globalY;
			border.w = this->width;
			border.h = this->height;
			SDL_SetRenderDrawColor(this->ren, 255, 255, 255, 255);
			SDL_RenderDrawRect(this->ren, &border);
		}

		int childX = mouseX - this->x;
		int childY = mouseY - this->y;
		int totalY = 0;

		if (this->hasDragBar)
		{
			SDL_Rect bar;
			bar.x = globalX;
			bar.y = globalY;
			bar.w = this->width;
			bar.h = 15;
			SDL_SetRenderDrawColor(this->ren, 255, 255, 255, 255);
			SDL_RenderDrawRect(this->ren, &bar);
			totalY = bar.h;
		}

		for (int i=0;i<this->children.size();i++)
		{
			this->children[i]->render(globalX + this->children[i]->x, globalY + totalY, childX, childY);
			totalY += this->children[i]->height;
		}
	}
};

class SystemMenu
{
	int x;
	int y;
	SDL_Renderer* ren;

	SystemMenu(SDL_Renderer* argRen, int argX, int argY)
	{
		this->ren = argRen;
		this->x = argX;
		this->y = argY;
	}

	void render(int mouseX, int mouseY)
	{
		
	}
};

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

struct __attribute__ ((packed)) nbody
{
	cl_double x;
	cl_double y;
	cl_double velX;
	cl_double velY;
	cl_double radius;
	cl_int mass;
	bool staticBody;
	bool dead;
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
	newNBody.radius = 1.0;
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

vector<nbody> updateBodies(vector<nbody> nbodyList, cl::Program program, cl::Device default_device, cl::Context context, cl::CommandQueue queue, cl::Buffer buffer_A, cl::Buffer buffer_C, cl::Buffer buffer_N)
{

	// apparently OpenCL only likes arrays ...
	// N holds the number of elements in the vectors we want to add
	int N[1] = {nbodyList.size()};
	int n = N[0];

	// push write commands to queue
	queue.enqueueWriteBuffer(buffer_A, CL_TRUE, 0, sizeof(nbody)*n, &nbodyList[0]);
	queue.enqueueWriteBuffer(buffer_N, CL_TRUE, 0, sizeof(int),   N);

	nbody C[n] = {0};
	queue.enqueueWriteBuffer(buffer_C, CL_TRUE, 0, sizeof(nbody)*n, C);

	// RUN ZE KERNEL
	cl::Kernel simple_add(program, "simple_add");
	simple_add.setArg(0, buffer_A);
	simple_add.setArg(1, buffer_C);
	simple_add.setArg(2, buffer_N);
	queue.enqueueNDRangeKernel(simple_add, cl::NullRange, cl::NDRange(max(1, n)), cl::NullRange);
	queue.finish();

	// read result from GPU to here
	queue.enqueueReadBuffer(buffer_C, CL_TRUE, 0, sizeof(nbody)*n, C);

	for (int i=0;i<nbodyList.size();i++)
	{
		nbodyList[i].x = C[i].x;
		nbodyList[i].y = C[i].y;
		nbodyList[i].velX = C[i].velX;
		nbodyList[i].velY = C[i].velY;
		nbodyList[i].mass = C[i].mass;
		nbodyList[i].radius = C[i].radius;
		nbodyList[i].dead = C[i].dead;
	}

	return nbodyList;
}

void saveNBodyList(vector<nbody>* nbodyList)
{
	ofstream f;
	f.open("nbody.csv", ios::out);

	for (int i=0;i<nbodyList->size();i++)
	{
		nbody curBody = nbodyList->at(i);
		if (!curBody.dead)
			f << curBody.x << "," << curBody.y << "," << curBody.velX << "," << curBody.velY << "," << curBody.radius << "," << curBody.mass << "\n";

	}

	f.close();
}

void loadNBodyList(vector<nbody>* nbodyList)
{
	nbodyList->clear();

	ifstream f;
	f.open("nbody.csv", ios::in);

	string line;
	while (getline(f, line))
	{
		nbody newBody;
		newBody.x = atof(strtok((char*)line.c_str(), ","));
		newBody.y = atof(strtok(nullptr, ","));
		newBody.velX = atof(strtok(nullptr, ","));
		newBody.velY = atof(strtok(nullptr, ","));
		newBody.radius = atof(strtok(nullptr, ","));
		newBody.mass = atoi(strtok(nullptr, ","));
		newBody.dead = false;
		newBody.staticBody = false;
		nbodyList->push_back(newBody);
	}

	f.close();
}

void printTotalMomentum(vector<nbody>* nbodyList)
{
	double momentumX = 0.00, momentumY = 0.00, tMomentum = 0.00;
	for (int i=0;i<nbodyList->size();i++)
	{
		nbody m = nbodyList->at(i);
		momentumX = m.mass*m.velX;
		momentumY = m.mass*m.velY;
		tMomentum += sqrt(momentumX*momentumX + momentumY*momentumY);
	}

	cout << tMomentum << endl;
	cout << "----" << endl;
}

int main(int argc, char** argv)
{
	using namespace std::chrono;

	// get all platforms (drivers), e.g. NVIDIA
	std::vector<cl::Platform> all_platforms;
	cl::Platform::get(&all_platforms);

	if (all_platforms.size()==0) {
		std::cout<<" No platforms found. Check OpenCL installation!\n";
		exit(1);
	}
	cl::Platform default_platform=all_platforms[0];
	std::cout << "Using platform: "<<default_platform.getInfo<CL_PLATFORM_NAME>()<<"\n";

	// get default device (CPUs, GPUs) of the default platform
	std::vector<cl::Device> all_devices;
	default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
	if(all_devices.size()==0){
		std::cout<<" No devices found. Check OpenCL installation!\n";
		exit(1);
	}

	// use device[1] because that's a GPU; device[0] is the CPU
	cl::Device default_device=all_devices[0];
	std::cout<< "Using device: "<<default_device.getInfo<CL_DEVICE_NAME>()<<"\n";

	// a context is like a "runtime link" to the device and platform;
	// i.e. communication is possible
	cl::Context context({default_device});

	// create the program that we want to execute on the device
	cl::Program::Sources sources;

	// calculates for each element; C = A + B
	std::string kernel_code=
		"typedef struct __attribute__ ((packed)) {"
		"	double x;"
		"	double y;"
		"	double velX;"
		"	double velY;"
		"	double radius;"
		"	int mass;"
		"	bool staticBody;"
		"   bool dead;"
		"} nbody;"
		""
		"   void kernel simple_add(global const nbody* A, global nbody* C, global const int* N) {"
		"       int ID, Nthreads, n, ratio, start, stop;"
		"		double timeStep, G;"
		""
		"       ID = get_global_id(0);"
		"       Nthreads = get_global_size(0);"
		"		n = N[0];"
		""
		"       ratio = (n / Nthreads);"  // number of elements for each thread
		"       start = ratio * ID;"
		"       stop  = ratio * (ID + 1);"
		"		timeStep = .1;"
		"		G = 1;"
		"       for (int i=start; i<stop; i++){"
		"           nbody curBody = A[i];"
		"			if (C[i].dead) continue;"
		"			for (int t=0; t < n; t++)"
		"			{"
		"				if (i != t)"
		"				{"
		"					nbody target = A[t];"
		"					double distX = target.x - curBody.x;"
		"					double distY = target.y - curBody.y;"
		"					double totalDist = sqrt(distX*distX + distY*distY);"
		"					if (totalDist == 0) continue;"
		""
		"					bool withinRange = totalDist < target.radius || totalDist < curBody.radius;"
		"					if ((withinRange && (curBody.mass >= target.mass || curBody.staticBody)) && !target.staticBody)"
		"					{"
		"						if (curBody.mass == target.mass && i < t)"
		"							continue;"
		"						curBody.velX = (curBody.mass*curBody.velX + target.mass*target.velX)/(curBody.mass+target.mass);"
		"						curBody.velY = (curBody.mass*curBody.velY + target.mass*target.velY)/(curBody.mass+target.mass);"
		"						curBody.mass += target.mass;"
		"						curBody.radius = cbrt(target.radius*target.radius*target.radius + curBody.radius*curBody.radius*curBody.radius);"
		"						C[t].dead = 1;"
		"					}"
		"					else"
		"					{"
		"						double accel = target.mass*G/(totalDist*totalDist);"
		"						double accX = accel * distX/totalDist;"
		"						double accY = accel * distY/totalDist;"
		"						curBody.velX += accX*timeStep;"
		"						curBody.velY += accY*timeStep;"
		"					}"
		"				}"	
		"			}"
		""
		"			if (curBody.staticBody){"
		"				curBody.velX = 0;"
		"				curBody.velY = 0;"
		"			}"
		""			
		"           C[i].velX = curBody.velX;"
		"           C[i].velY = curBody.velY;"
		" 			C[i].x = curBody.x + curBody.velX*timeStep;"
		"			C[i].y = curBody.y + curBody.velY*timeStep;"
		"			C[i].mass = curBody.mass;"
		"			C[i].radius = curBody.radius;"
		"		}"
		"   }";
	
	sources.push_back({kernel_code.c_str(), kernel_code.length()});

	cl::Program program(context, sources);
	if (program.build({default_device}) != CL_SUCCESS) {
		std::cout << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;
		exit(1);
	}

	// create a queue (a queue of commands that the GPU will execute)
	cl::CommandQueue queue(context, default_device);

	// create buffers on device (allocate space on GPU)
	cl::Buffer buffer_A(context, CL_MEM_READ_WRITE, sizeof(nbody) * 100000);
	cl::Buffer buffer_C(context, CL_MEM_READ_WRITE, sizeof(nbody) * 100000);
	cl::Buffer buffer_N(context, CL_MEM_READ_ONLY,  sizeof(int));

	bool running = true;
	SDL_Event event;
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window *mainWin = SDL_CreateWindow("NBODY SIM", 100, 100, 1000, 720, SDL_WINDOW_SHOWN);
	SDL_Renderer *ren = SDL_CreateRenderer(mainWin, -1, SDL_RENDERER_ACCELERATED);

	vector<nbody> nbodyList;

	TTF_Init();

	MenuItem mainMenu(ren, 50, 50, 100, 30);
	mainMenu.setDragBar(true);
	for (int i=0;i<4;i++)
	{
		MenuItem* item = new MenuItem(ren, 0, 0, 100, 30, 1);
		item->setText("test " + to_string(i));
		mainMenu.children.push_back(item);
	}

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
	high_resolution_clock::time_point lastTime = high_resolution_clock::now();
	vector<duration<double>> timeSamples;
	while (running)
	{
		// timeSamples.push_back(high_resolution_clock::now()-lastTime);
		// if (timeSamples.size() > 30)
		// {
		// 	timeSamples.erase(timeSamples.begin());

		// 	double avgDur;
		// 	for (int i=0;i<timeSamples.size();i++)
		// 	{
		// 		avgDur += timeSamples[i].count();
		// 	}
		// 	avgDur /= timeSamples.size();

		// 	//TTF_Font* Sans = TTF_OpenFont("Sans.ttf", 24); //this opens a font style and sets a size

		// 	//SDL_Color White = {255, 255, 255};  // this is the color in rgb format, maxing out all would give you the color white, and it will be your text's color
		// 	//SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, ("FPS: " + to_string(1/avgDur)).c_str(), White); // as TTF_RenderText_Solid could only be used on SDL_Surface then you have to create the surface first
		// 	//SDL_Texture* Message = SDL_CreateTextureFromSurface(ren, surfaceMessage); //now you can convert it into a texture
		// 	//SDL_Rect Message_rect; //create a rect
		// 	//Message_rect.x = 0;  //controls the rect's x coordinate 
		// 	//Message_rect.y = 0; // controls the rect's y coordinte
		// 	//Message_rect.w = 100; // controls the width of the rect
		// 	//Message_rect.h = 100; // controls the height of the rect
		// 	//SDL_RenderCopy(ren, Message, NULL, &Message_rect);

		// 	//cout << 1/avgDur << " frames per second" << endl;
		// }
		// lastTime = high_resolution_clock::now();

		SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(ren);
		int width;
		int height;
		SDL_GetWindowSize(mainWin, &width, &height);
		double centerX = cameraOffsetX + (double)width/2;
		double centerY = cameraOffsetY + (double)height/2;

		//Apply gravitational acceleration between all bodies
		//Combine bodies that have moved too close to one another (perfectly elastic collision)
		nbodyList = updateBodies(nbodyList, program, default_device, context, queue, buffer_A, buffer_C, buffer_N);

		//printTotalMomentum(&nbodyList);

		for (int i=nbodyList.size()-1; i>=0; i--)
		{
			if (nbodyList[i].dead)
			{
				nbodyList.erase(nbodyList.begin() + i);
				continue;
			}
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
				SDL_Point normal;
				normal.x = -cameraOffsetX + curBody.x;
				normal.y = -cameraOffsetY + curBody.y;
				SDL_Point scaled;
				scaled.x = (normal.x - width/2)*scale + width/2;
				scaled.y = (normal.y - height/2)*scale + height/2;
				circleCoords = getCirclePoints(scaled.x, scaled.y, 20, curBody.radius*scale);
				vector<SDL_Point> velVec;
				SDL_Point start, end;
				start.x = scaled.x;
				start.y = scaled.y;
				end.x = scaled.x + curBody.velX;
				end.y = scaled.y + curBody.velY;
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

		int ret = SDL_PollEvent(&event);
		if (ret)
		{
			if (event.type == SDL_QUIT)
				running = false;
			else if (event.type == SDL_MOUSEWHEEL)
			{
				if(event.wheel.y > 0) // scroll up
				{
					scale *= .95;
				}
				else
				{
					scale /= .95;
				}
			}
			//else if (event.type == SDL_MOUSEBUTTONDOWN)
			//{
			//	bool ret = mainMenu.captureInput(event);
			//}
		}

		const Uint8* keystate = SDL_GetKeyboardState(NULL);
		int mouseX;
		int mouseY;
		double moveDiff = 3;
		Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);

		mainMenu.render(mainMenu.x, mainMenu.y, mouseX, mouseY);

		if (keystate[SDL_SCANCODE_C])
		{
			nbodyList.clear();
		}
		else if (keystate[SDL_SCANCODE_R])
		{
			cameraOffsetX = 0;
			cameraOffsetY = 0;
		}
		else if (keystate[SDL_SCANCODE_Z])
		{
			scale = 1;
		}
		else if (keystate[SDL_SCANCODE_LEFT])
		{
			cameraOffsetX -= moveDiff * 1/scale;
		}
		else if (keystate[SDL_SCANCODE_RIGHT])
		{
			cameraOffsetX += moveDiff * 1/scale;
		}
		else if (keystate[SDL_SCANCODE_UP])
		{
			cameraOffsetY -= moveDiff * 1/scale;
		}
		else if (keystate[SDL_SCANCODE_DOWN])
		{
			cameraOffsetY += moveDiff * 1/scale;
		}
		else if (keystate[SDL_SCANCODE_A] && !buttonFlag)
		{
			makeAccDisk(40000, height*10, 200000, mainWin, &nbodyList);
			buttonFlag = true;
		}
		else if (keystate[SDL_SCANCODE_F] && !buttonFlag)
		{
			saveNBodyList(&nbodyList);
			buttonFlag = true;
		}
		else if (keystate[SDL_SCANCODE_G] && !buttonFlag)
		{
			loadNBodyList(&nbodyList);
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
			placeRandomField(40000, 5, 10*height, mainWin, &nbodyList);
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

		SDL_Rect dest;
		dest.x = width/2;
		dest.y = height/2;
		dest.w = 100;
		dest.h = 100;

		//SDL_RenderCopy(ren, texture, NULL, &dest);
		//Update screen
		SDL_RenderPresent(ren);
		Sleep(1);
	}

	TTF_Quit();
	SDL_DestroyWindow(mainWin);
	SDL_Quit();
	return 0;
}


