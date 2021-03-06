#include "Partie1.h"
#include "util.h"
#include <string.h>

int octets=0;
int ExElements=0;

int MyIsbigEndian(Elf32_Ehdr h)
{
	return h.e_ident[5] == ELFDATA2MSB;
}

int32_t recuperer_valeur32(Elf32_Ehdr h, int32_t value)
{
	return MyIsbigEndian(h) ? __bswap_32(value) : value;
}

int16_t recuperer_valeur16(Elf32_Ehdr h, int16_t value)
{
	return MyIsbigEndian(h) ? __bswap_16(value) : value;
}

void fwrite_value16(FILE * f, Elf32_Ehdr h, int value, int size)
{
	int16_t v = recuperer_valeur16(h, value);
	fwrite(&v, size, 1, f);
}

void fwrite_value32(FILE * f, Elf32_Ehdr h, int value, int size)
{
	int32_t v = recuperer_valeur32(h, value);
	fwrite(&v, size, 1, f);
}

//On compte le nombre de séctions à supprimer (les séctions avec des types REL)

Elf32_Ehdr NbSectionASup(Elf32_Shdr* TableHsCopie,Elf32_Ehdr dataCopie, int* numSectionSuppr, int* nbSecSuppr){

	int cpt=0;

	for (int i = 0; i<dataCopie.e_shnum ; ++i)
	{
		if(TableHsCopie[i].sh_type == 9 || (TableHsCopie[i].sh_size == 0 && TableHsCopie[i].sh_type != 0)){
			numSectionSuppr[cpt] = i;
			cpt++;
		}
	}
	*nbSecSuppr = cpt;
	dataCopie.e_shnum -= cpt;
	return	dataCopie;
}

// On supprime les sections de type REL et on compte le nombre d'octets des ces sections
// On envoie une copie de table des sections en argument
//recupere le nouveau index du type e_shstrndx

void EditTabSection(Elf32_Shdr** TableHsCopie,Elf32_Shdr* TableHs,Elf32_Ehdr dataCopie,Elf32_Ehdr data){

	printf("data.e_shnum-dataCopie.e_shnum=%d\n",data.e_shnum-dataCopie.e_shnum );
	for (int i = 0; i<data.e_shnum ; ++i)
	{
		if(TableHs[i].sh_type == 9 || (TableHs[i].sh_size == 0 && TableHs[i].sh_type != 0)){

			octets+=(TableHs)[i].sh_size;
			for(int j=i-ExElements ;j<data.e_shnum-1;++j)
			{
				if(TableHs[i].sh_size > 0){

					if((TableHs[j+1+ExElements].sh_offset > TableHs[i].sh_offset))

						{

						(*TableHsCopie)[j+1].sh_offset=(*TableHsCopie)[j+1].sh_offset-TableHs[i].sh_size;
						if ((TableHs)[j].sh_link!=0)
						{
							(*TableHsCopie)[j+2+ExElements].sh_link =TableHs[j].sh_link-(data.e_shnum-dataCopie.e_shnum)+1;
						}
						if ((TableHs)[j].sh_info!=0)
						{
							(*TableHsCopie)[j+2+ExElements].sh_info = (*TableHsCopie)[j+2+ExElements].sh_info-1;
						}

					}
				}
				(*TableHsCopie)[j] = (*TableHsCopie)[j+1];

			}
			ExElements++;
		}
	}
	printf("dataCopie.e_shnum = %d\n",dataCopie.e_shnum );

	//TableHsCopie[dataCopie.e_shnum -2].sh_link = TableHsCopie[dataCopie.e_shnum -1]
}

void writeHeader(FILE *f, Elf32_Shdr* TableHsCopie,Elf32_Ehdr dataCopie){


	//ecriture du Header
	//fwrite(dataCopie,1,sizeof(Elf32_Ehdr),SortieElf);
	rewind(f);

	fwrite(dataCopie.e_ident, sizeof(unsigned char), EI_NIDENT, f);
	fwrite_value16(f, dataCopie, dataCopie.e_type, sizeof(Elf32_Half));
	fwrite_value16(f, dataCopie, dataCopie.e_machine, sizeof(Elf32_Half));
	fwrite_value32(f, dataCopie, dataCopie.e_version, sizeof(Elf32_Word));
	fwrite_value32(f, dataCopie, dataCopie.e_entry, sizeof(Elf32_Addr));
	fwrite_value32(f, dataCopie, dataCopie.e_phoff, sizeof(Elf32_Off));
	fwrite_value32(f, dataCopie, dataCopie.e_shoff, sizeof(Elf32_Off));
	fwrite_value32(f, dataCopie, dataCopie.e_flags, sizeof(Elf32_Word));
	fwrite_value16(f, dataCopie, dataCopie.e_ehsize, sizeof(Elf32_Half));
	fwrite_value16(f, dataCopie, dataCopie.e_phentsize, sizeof(Elf32_Half));
	fwrite_value16(f, dataCopie, dataCopie.e_phnum, sizeof(Elf32_Half));
	fwrite_value16(f, dataCopie, dataCopie.e_shentsize, sizeof(Elf32_Half));
	fwrite_value16(f, dataCopie, dataCopie.e_shnum, sizeof(Elf32_Half));
	fwrite_value16(f, dataCopie, dataCopie.e_shstrndx, sizeof(Elf32_Half));

	//ecriture de l'en-têtes
	//fwrite(TableHsCopie,1,sizeof(TableHsCopie)*dataCopie.e_shnum-1,f);
	for (int i = 0; i < dataCopie.e_shnum; ++i)
	{
		fseek(f,dataCopie.e_shoff + sizeof(Elf32_Shdr)*i, SEEK_SET);

	fwrite_value32(f, dataCopie, TableHsCopie[i].sh_name, sizeof(Elf32_Word));
	fwrite_value32(f, dataCopie, TableHsCopie[i].sh_type, sizeof(Elf32_Word));
	fwrite_value32(f, dataCopie, TableHsCopie[i].sh_flags, sizeof(Elf32_Word));
	fwrite_value32(f, dataCopie, TableHsCopie[i].sh_addr, sizeof(Elf32_Addr));
	fwrite_value32(f, dataCopie, TableHsCopie[i].sh_offset, sizeof(Elf32_Off));
	fwrite_value32(f, dataCopie, TableHsCopie[i].sh_size, sizeof(Elf32_Word));
	fwrite_value32(f, dataCopie, TableHsCopie[i].sh_link, sizeof(Elf32_Word));
	fwrite_value32(f, dataCopie, TableHsCopie[i].sh_info, sizeof(Elf32_Word));
	fwrite_value32(f, dataCopie, TableHsCopie[i].sh_addralign, sizeof(Elf32_Word));
	fwrite_value32(f, dataCopie, TableHsCopie[i].sh_entsize, sizeof(Elf32_Word));
	}
}

void SupprimerSection(Elf32_Shdr* TableHs,Elf32_Ehdr data,FILE *fichier){



	int j;
	void* str=NULL;

	//pour la correction des symboles
	int nbSecSuppr = 0;
	int numSectionSuppr[data.e_shnum];

	Elf32_Shdr* TableHsCopie;
	Elf32_Ehdr dataCopie;
	FILE* resultat=fopen("SortieElf","w+");
	TableHsCopie = (Elf32_Shdr*)malloc(sizeof(Elf32_Shdr)*data.e_shnum);

	dataCopie=data;

	for (int i = 0; i < data.e_shnum; ++i)
	{
			memcpy(&(TableHsCopie[i]), &(TableHs[i]),sizeof(Elf32_Shdr));
	}

	dataCopie = NbSectionASup(TableHsCopie,dataCopie,numSectionSuppr, &nbSecSuppr);
	printf("dataCopie.e_shnum2 = %d\n",data.e_shnum );
	EditTabSection(&TableHsCopie,TableHs,dataCopie,data);
	dataCopie.e_shstrndx -=ExElements;
	writeHeader(resultat,TableHsCopie,dataCopie);

	for(int i=0;i<dataCopie.e_shnum;i++){
		j=i;
		while(TableHsCopie[i].sh_name!=TableHs[j].sh_name){
			j++;
		}
		//ecriture de la section
		if(TableHs[j].sh_type == SHT_SYMTAB){
			str = malloc(TableHs[j].sh_size*sizeof(char));
			fseek(fichier,TableHs[j].sh_offset,SEEK_SET);

			//on récupère le nombre de symbole
      int nbSymbole = TableHs[j].sh_size/sizeof(Elf32_Sym);

			//on initialise notre tableau de symbole
      Elf32_Sym symTab[nbSymbole] ;
      int tailleSymTab = sizeof(symTab);
      readElfFileSymTable(fichier, TableHs[j], symTab, nbSymbole, tailleSymTab);

			//correction symboles
			for (int k = 0; k < nbSecSuppr; k++) {
				for (int h = 0; h < nbSymbole; h++) {
					if (strcmp(get_symbol_type(ELF32_ST_TYPE(symTab[h].st_info)),"SECTION")==0 ) {
						if (symTab[h].st_shndx > numSectionSuppr[k]) {
							symTab[h].st_shndx--;
						}
					}
				}
				for (int u = k; u < nbSecSuppr; u++) {
					numSectionSuppr[u]--;
				}
			}

			//affichage du tableau de symbole
			printf("\nTable de symbole après modification de l'index:\n" );
			printf("%-10s%-10s%-10s%-20s%-10s%-10s%-10s%-10s\n", "Num","Valeur","Tail","Type","Lien","Vis","Ndx","Nom");
			for (int g = 0; g < nbSymbole; g++) {
        printf("%-10d:  ",g);
        printf("%-10.8x    ",symTab[g].st_value);
        printf("%-10d    ",symTab[g].st_size);
        printf("%-10s", get_symbol_type(ELF32_ST_TYPE(symTab[g].st_info)));
        printf("%-10s", findSymLink(symTab[g]));
        printf("%-10s",get_symbol_visibility(symTab[g].st_other));
        if (symTab[g].st_shndx == 0) {
          printf("%-10s","UND");
        }else{
          printf("%-10d",symTab[g].st_shndx);
        }
        //printf("%-10s    ",getName(symTab[i].st_name, tabHeadSection[numStringTable],fichier));
        printf("%-10d", symTab[g].st_name);
        printf("\n");
      }
			printf("\n");

			//reverse_endianess
			for (int g = 0; g < nbSymbole; g++) {
	      symTab[g] = reverseAllEndiannessSym(symTab[g]);
	    }

			//on recopie dans notre fichier de sortie
			fwrite(symTab,sizeof(Elf32_Sym),nbSymbole,resultat);

		}else{
			//ecriture de la section
			str = malloc(TableHs[j].sh_size*sizeof(char));
			fseek(fichier,TableHs[j].sh_offset,SEEK_SET);
			fread(str,sizeof(char),sizeof(char)*TableHs[j].sh_size,fichier);
			fwrite(str,sizeof(char),sizeof(char)*TableHs[j].sh_size,resultat);
		}
	}

	//displayElfFileHeader("SortieElf");
	//afficherHeader(TableHsCopie,&dataCopie,fichier);
	//afficherSection(fichier,TableHsCopie[8],"8");
	fclose(resultat);
}
