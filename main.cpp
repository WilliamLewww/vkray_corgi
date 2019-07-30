#include "src/engine.h"

Engine* engine;

int main() {
	engine = new Engine;
	engine->initialize();
	engine->start();
	engine->quit();

	delete engine;

	return 0;
}