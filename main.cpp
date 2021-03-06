#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
//#include <GLES2/gl2.h>
#include <SDL2/SDL_opengles2.h>


#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include "game.hpp"

#include <iostream>
#include <string>

// Whether we should quit. It is most convenient for this to be filescope right now.
static bool done = false;

std::string align(std::string in, int num_tabs) {
	for (int i = (num_tabs * 8 - in.size()); i > 0; --i) {
		in += " ";
	}
	return in;
}

void log_gl_value(const std::string & name, SDL_GLattr sdl_enum) {
	int val;
	if (SDL_GL_GetAttribute(sdl_enum, &val)) { std::cerr << "(error?) "; }
	std::cout << name << val << std::endl;
}

void output_gl_diagnostics() {
	auto gl_version 	= reinterpret_cast< char const * >(glGetString(GL_VERSION));
	auto gl_renderer 	= reinterpret_cast< char const * >(glGetString(GL_RENDERER));
	auto gl_vendor	 	= reinterpret_cast< char const * >(glGetString(GL_VENDOR));
	auto glsl_version 	= reinterpret_cast< char const * >(glGetString(GL_SHADING_LANGUAGE_VERSION));
	auto gl_extensions	= reinterpret_cast< char const * >(glGetString(GL_EXTENSIONS));

	if (!(gl_version && gl_renderer && gl_vendor && glsl_version)) {
		std::cerr << "Could not obtain complete GL version info... GL context might not exist?!?\n";
	}

	std::cout << align("GL version:", 3) << (gl_version ? gl_version : "(null)") << std::endl;
	std::cout << align("GL renderer:", 3) << (gl_renderer ? gl_renderer : "(null)") << std::endl;
	std::cout << align("GL vendor:", 3) << (gl_vendor ? gl_vendor : "(null)") << std::endl;
	std::cout << align("GLSL version:", 3) << (glsl_version ? glsl_version : "(null)") << std::endl;
	std::cout << align("GL extensions:", 3) << (gl_extensions ? gl_extensions : "(null)") << std::endl;
	log_gl_value(align("Red Size:", 3), SDL_GL_RED_SIZE);
	log_gl_value(align("Blue Size:", 3), SDL_GL_BLUE_SIZE);
	log_gl_value(align("Green Size:", 3), SDL_GL_GREEN_SIZE);
	log_gl_value(align("Alpha Size:", 3), SDL_GL_ALPHA_SIZE);
	log_gl_value(align("Depth Size:", 3), SDL_GL_DEPTH_SIZE);
	log_gl_value(align("Double Buffer:", 3), SDL_GL_DOUBLEBUFFER);
}

struct SDL_graphics {
	int width_;
	int height_;
	SDL_Window * window_;
	SDL_GLContext context_;
//	SDL_Renderer * renderer_;

	SDL_graphics(int width, int height)
		: width_(width)
		, height_(height)
		, window_(nullptr)
		, context_(nullptr)
//		, renderer_(nullptr)
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); 
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0); 
		// Probably not necessary but can't hurt

//		SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_OPENGL, &window_, &renderer_);
		window_ = SDL_CreateWindow("DEMO", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
				           SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE /*| SDL_WINDOW_ALLOW_HIGHDPI*/);
		if (window_ == nullptr) { std::cerr << "Error when creating SDL GL Window: '" << SDL_GetError() << "'\n"; }

		context_ = SDL_GL_CreateContext(window_);
		if (!context_) { std::cerr << "Error when creating SDL GL Context: '" << (SDL_GetError()) <<"'\n"; }


		int w, h;
		SDL_GetWindowSize(window_, &w, &h);		

		output_gl_diagnostics();
	}
};

static std::string locate_assets();

struct program {
	SDL_graphics graphics_;
	std::unique_ptr<game> game_;

	program(int width, int height)
		: graphics_(width, height)
		, game_(new game())
	{
	}
};

static Uint32 last_time = 0;

void loop_iteration(program* prog)
{
	// Now render graphics
	{
		glClear(GL_COLOR_BUFFER_BIT);

		prog->game_->draw();

		// Now flip the frame
		SDL_GL_SwapWindow(prog->graphics_.window_); //->swap_buffers();
	}

	done |= SDL_QuitRequested();
}


int main(int argc, char* argv[])
{
//	SDL_Init(SDL_INIT_VIDEO);
		SDL_SetMainReady(); // Note: Emscripten crashes if SDL_INIT_TIMER is passed here
		if (SDL_Init(/*SDL_INIT_TIMER |*/ SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
			std::cerr << "SDL_Init failed!\n";
			std::cerr << SDL_GetError() << std::endl;
			throw 42;
		}

	int width = 800, height = 600;

	// The mechanism of "emscripten_set_main_loop" is a bit opaque, better not to make this a stack object.
	program * me = new program(width, height);

	// Now enter main loop
	last_time = SDL_GetTicks();

#ifdef EMSCRIPTEN
	emscripten_set_main_loop_arg((em_arg_callback_func)loop_iteration, me, 0, 1);
#else
	while (!done) {
		loop_iteration(me);
	}
        delete me;
#endif


	SDL_Quit();

	return 0;
}
