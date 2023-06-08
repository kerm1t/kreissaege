// Filter.cpp: Implementierung der Klasse Filter.
//
//////////////////////////////////////////////////////////////////////

#include "Filter.h"

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

Filter::Filter()
{

}

Filter::~Filter()
{

}

void Filter::median()
//void Filter::median(std::vector v_in, int rang)
{
/*	int i,j;
	int size = this->data.size();
	std::vector<int> werte;

	std::vector<int> output(size,0);

	for (i=rang/2;i<size-rang/2;i++) {
		werte.clear();
		for (j=0;j<rang;j++) {
			werte.push_back(this->data[i+j]);
//			std::cout << this->data[i+j] << " ";
		}
		// sortieren und das mittlere Element nehmen
		std::sort(werte.begin(),werte.end());
//		std::cout << "; " << werte[(int)rang/2] << std::endl;
		output[i] = werte[(int)rang/2];
	}
	this->data = output;
*/}

void Filter::gauss()
{

}

void Filter::mittel()
{

}
