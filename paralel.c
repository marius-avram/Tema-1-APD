#include <stdio.h> 
#include <string.h>
#include <stdlib.h> 
#include <math.h>
#include <limits.h>
#include <omp.h> 

typedef struct {
	int resourceA, resourceB; 
	int maxA, maxB;
} market;

int error(char* message){
	printf("%s\n", message);
	return -1; 
}

/* Pentru a folosi functia presupunem deja ca s-au 
 * dat numarul necesar de parametri. Returneaza cod 
 * de eroare in cazul in care parametrii nu pot fi
 * parsati. De exemplu parametrul T nu este int */
int parse_arguments(char** argv, int* iterations, int* matrix_size, int* version){
	char *fisin = argv[2];
	char size[10], versionc[10];
	int index = 0;
	char common_msg[256] = "fisin trebuie sa fie de forma inN_varianta.txt";
	
	*iterations = atoi(argv[1]);
	if (*iterations == 0){
		return error("Primul argument trebuie sa fie intreg."); 
	}
	// Verificam ca numele fisierului de intrare respecta formatul
	if (strncmp(fisin, "in", 2) != 0 || strchr(fisin, '_') == NULL || 
		strstr(fisin,".txt") == NULL){
		return error(common_msg);
	}
	
	// Aflam dimensiunea matricii
	fisin += 2;
	while (*fisin != '_'){
		size[index] = *fisin;
		fisin++;
		index++;
	}
	
	// Numele fisierului de intrare nu contine dimenisunea matricii N
	if (index == 0){
		return error(common_msg);
	}
	
	size[index] = '\0';
	*matrix_size = atoi(size);
	if (*matrix_size == 0){
		return error("Dimenisunea matricii din numele fisierului = 0");
	} 
	
	index = 0;
	fisin = argv[2];
	fisin = strchr(fisin,'_') + 1;
	while(*fisin != '.'){
		versionc[index] = *fisin;
		fisin++; 
		index++;
	}
	
	// Numele fisierului de intrare nu contine varianta
	if (index == 0){
		return error(common_msg);
	}
	
	versionc[index] = '\0';
	*version = atoi(versionc);
	
	return 0;
}

/* Creeaza numele fisierului de iesire si verifica daca nu cumva acesta 
 * este diferit de cel dat de catre utilizator. Returneaza de asemenea 
 * mesaj de eroare daca cele doua char* nu se aseamna */
int check_output_filename(char** argv, int iterations, 
						  int matrix_size, int version){

	char filename[100];
	sprintf(filename, "out%i_%i_%i.txt", matrix_size, version, iterations);
	if (strcmp(filename, argv[3])){
		return error("Numele fisierului de iesire nu respecta formatul.");
	}
	else {
		return 0;
	}
} 

int main(int argc, char** argv){
	int iterations, matrix_size, version,
		pmin, pmax, n;
	int result, i, j, k, l, dist, cost, cost_min, budget, index, own_cost_min;
	int resourceA, resourceB;
	FILE *p_filein, *p_fileout;
	
	if (argc < 4){
		return error("Mod de rulare ./serial T fisin fisout");
	}
	
	result = parse_arguments(argv, &iterations, &matrix_size, &version);
	if (result != 0){
		return result;
	}

	
	result = check_output_filename(argv, iterations, matrix_size, version);
	if (result != 0){
		return result;
	}
	
	// Declararea matricilor a caror dimensiune depinde de parametrii dati
	int resources[matrix_size][matrix_size], 
		prices[matrix_size][matrix_size],
		budgets[matrix_size][matrix_size],
		// tin informatii despre anul anterior celui analizat curent
		resources_prev[matrix_size][matrix_size],
		prices_prev[matrix_size][matrix_size],
		budgets_prev[matrix_size][matrix_size];
	// Declararea vectorului cu informatii despre piata in fiecare an
	market market_info[iterations];
	
	// Citirea din fisier
	p_filein = fopen(argv[2], "r");
	if (p_filein == NULL){
		return error("Fisierul de intrare nu a putut fi deschis");
	}
	
	fscanf(p_filein,"%i %i %i", &pmin, &pmax, &n);
	if (n != matrix_size) {
		printf("Warning: Dimensiunea matricii are valori diferite:");
		printf("N din inN_versiune.txt <> de valoarea din fisier\n");
	}
	
	// Citirea resurselor
	for(i=0; i<matrix_size; ++i){
		for(j=0; j<matrix_size; ++j){
			fscanf(p_filein,"%i", &resources_prev[i][j]);
		}
	}
	
	// Citirea matricii cu preturi
	for(i=0; i<matrix_size; ++i){
		for(j=0; j<matrix_size; ++j){
			fscanf(p_filein,"%i", &prices_prev[i][j]);
			if (prices_prev[i][j] < pmin || prices_prev[i][j] > pmax)
				return error("Matricea preturilor nu are valori din [pmin,pmax]");
		}
	}
	
	// Citirea matricii cu bugete 
	for(i=0; i<matrix_size; ++i){
		for(j=0; j<matrix_size; ++j){
			fscanf(p_filein,"%i", &budgets_prev[i][j]); 
		}
	}
	
	fclose(p_filein);
	
	
	// Aplicarea algoritmului serial timp de n iteratii
	index = 0;
	while(index != iterations){
		
		market_info[index].maxA = INT_MIN; 
		market_info[index].maxB = INT_MIN;
		resourceA = 0;
		resourceB = 0;
		
		#pragma omp parallel for schedule(runtime) \
		 collapse(2)\
		 private(cost_min, own_cost_min, dist, cost, budget, i, j, k, l) \
		 reduction(+:resourceA, resourceB)
		for(i=0; i<matrix_size; i++){
			for(j=0; j<matrix_size; j++){
				//printf("thread=%i, i=%i, j=%i\n", omp_get_thread_num(), i, j);
				// Pentru fiecare colonist in parte analizam preturile 
				// si luam decizia cea mai avantajoasa
				cost_min = INT_MAX;
				own_cost_min = INT_MAX;
				for(k=0; k<matrix_size; k++){
					for(l=0; l<matrix_size; l++){
					
						// Calculam costul minim pentru resursa proprie 
						// si pentru celalt tip de resursa din perspectiva
						// colonistului curent i,j
						dist = abs(i-k) + abs(j-l);
						cost = prices_prev[k][l] + dist;
						
						if (cost<cost_min && 
							resources_prev[i][j]!=resources_prev[k][l]){
							cost_min = cost;
						}
						
						if (cost<own_cost_min && 
							resources_prev[i][j]==resources_prev[k][l]){
							own_cost_min = cost;
						}
					
						
					}
				}
			
				// Recalculam bugetul si pretul pentru produsul propriu.
				budget = budgets_prev[i][j];
				resources[i][j] = resources_prev[i][j];

				if (cost_min > budget){
					budgets[i][j] = cost_min;
					prices[i][j] = prices_prev[i][j] + cost_min - budget;
				}
				else if (cost_min < budget){
					budgets[i][j] = cost_min; 
					prices[i][j] = prices_prev[i][j] + (cost_min - budget)/2;
				}
				else if (cost_min == budget){
					budgets[i][j] = cost_min; 
					prices[i][j] = own_cost_min + 1;
				}
			
				if (prices[i][j] < pmin){
					prices[i][j] = pmin;
				}
				else if (prices[i][j] > pmax){
					resources[i][j] = !resources_prev[i][j];
					budgets[i][j] = pmax;
					prices[i][j] = (pmin + pmax) / 2;
				}
				
			// Retinerea informatiilor despre piata in noul an
			resources[i][j] == 0 ? resourceA++: resourceB++;
			// Numai un singur thread poate modifica un maxim in acelasi timp
			#pragma omp critical(maxA)
			if (resources[i][j]==0 && prices[i][j]>market_info[index].maxA){
					market_info[index].maxA = prices[i][j];
			}
			#pragma omp critical(maxB) 
			if (resources[i][j]==1 && prices[i][j]>market_info[index].maxB){
					market_info[index].maxB = prices[i][j];
			}
			
			}
		}
		
		market_info[index].resourceA = resourceA; 
		market_info[index].resourceB = resourceB; 
		
		// Dupa terminarea unui ciclu trebuie ca valorile anului 
		// anterior sa primeasca valorile anului curent
		for(k=0; k<matrix_size; ++k){
			for(l=0; l<matrix_size; ++l){
				resources_prev[k][l] = resources[k][l];
				prices_prev[k][l] = prices[k][l];
				budgets_prev[k][l] = budgets[k][l];
			}
		}
		index++;
	}
	
	
	
	/* Se afiseaza rezultatul in fisier */
	p_fileout = fopen(argv[3], "w");
	if (p_fileout == NULL){
		error("Fisierul de iesire nu a putut fi deschis/creat.");
	}
	
	// Informatii generale despre fiecare an
	for(i=0; i<iterations; ++i){
		fprintf(p_fileout, "%i %i %i %i\n", market_info[i].resourceA, 
				market_info[i].maxA, market_info[i].resourceB,
				market_info[i].maxB);
	}
	
	// Cele 3 matrici in ultimul an
	for(i=0; i<matrix_size; ++i){
			for(j=0; j<matrix_size; ++j){
				fprintf(p_fileout,"(%i,%i,%i) ",resources[i][j], prices[i][j], 
						budgets[i][j]);
			}
			fprintf(p_fileout,"\n");
	}
	
	fclose(p_fileout);
	
	return 0;
}
