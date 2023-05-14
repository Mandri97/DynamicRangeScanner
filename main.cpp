#include <iostream>
#include "audio.hh"

// Forward declaration
void usage(const char *fname);

int main(int argc, char *argv[]){
	// check argument
	if (argc != 2) usage(argv[0]);
	const std::string inputfile = argv[1];

	Audio *audio = new Audio();

	audio->load_audio(inputfile);
	audio->print_metadata();
	audio->decode();

	return 0;
}

void usage(const char *fname){
	fprintf(stderr, "Missing argument\n");
	fprintf(stdout, "Usage:./%s inputfile\n", fname);

	exit(EXIT_FAILURE);
}
