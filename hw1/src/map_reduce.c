//**DO NOT** CHANGE THE PROTOTYPES FOR THE FUNCTIONS GIVEN TO YOU. WE TEST EACH
//FUNCTION INDEPENDENTLY WITH OUR OWN MAIN PROGRAM.
#include "map_reduce.h"
#include <string.h>

//Implement map_reduce.h functions here.
int validateargs(int argc, char** argv){ //2, 3, 4 arguments
	if(argc > 2 || argc < 4){
		printf("Usage: ./mapreduce [h|v] FUNC DIR \n");
		printf("FUNC        Which operation you would like to run on the data:\n");
		printf("ana - Analysis of various text files in a directory.\n");
		printf("stats - Calculates stats on files which contain only numbers.\n");
		printf("DIR         The directory in which the files are located.\n \n");
		printf("Options: \n");
		printf("-h          Prints this help menu.\n");
		printf("-v          Prints the map function's results, stating this file it's from.\n");
		return EXIT_FAILURE; //-1
	}

	if (strcmp(argv[1], "-h")){
		printf("Usage: ./mapreduce [h|v] FUNC DIR \n");
		printf("FUNC        Which operation you would like to run on the data:\n");
		printf("ana - Analysis of various text files in a directory.\n");
		printf("stats - Calculates stats on files which contain only numbers.\n");
		printf("DIR         The directory in which the files are located.\n \n");
		printf("Options: \n");
		printf("-h          Prints this help menu.\n");
		printf("-v          Prints the map function's results, stating this file it's from.\n");
		return EXIT_SUCCESS; //or 0
	}
	if(strcmp(argv[1], "ana")){
		return 1;
	}
	if(strcmp(argv[1], "stats")){
		return 2;
	}
	if(strcmp(argv[1], "-v")){
		if(strcmp(argv[2], "ana"))
			return 3;
		if(strcmp(argv[2], "stats"))
			return 4;
	}

	printf("Usage: ./mapreduce [h|v] FUNC DIR \n");
	printf("FUNC        Which operation you would like to run on the data:\n");
	printf("ana - Analysis of various text files in a directory.\n");
	printf("stats - Calculates stats on files which contain only numbers.\n");
	printf("DIR         The directory in which the files are located.\n \n");
	printf("Options: \n");
	printf("-h          Prints this help menu.\n");
	printf("-v          Prints the map function's results, stating this file it's from.\n");
	return EXIT_FAILURE; //-1
}

