/*
Copyright (C) 2019-2020 Andrei Kopanchuk UZ7HO

This file is part of QtSoundModem

QtSoundModem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QtSoundModem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QtSoundModem.  If not, see http://www.gnu.org/licenses

*/

// UZ7HO Soundmodem Port by John Wiseman G8BPQ

#include "UZ7HOStuff.h"

/*{***********************************************************************
*                                                                      *
*       RSUnit.pas                                                     *
*                                                                      *
*       (C) Copyright 1990-1999 Bruce K. Christensen                   *
*                                                                      *
*       Modifications                                                  *
*       =============                                                  *
*                                                                      *
***********************************************************************}

{ This program is an encoder/decoder for Reed-Solomon codes. Encoding
  is in systematic form, decoding via the Berlekamp iterative algorithm.
  In the present form , the constants mm, nn, tt, and kk=nn-2tt must be
  specified  (the double letters are used simply to avoid clashes with
  other n,k,t used in other programs into which this was incorporated!)
  Also, the irreducible polynomial used to generate GF(2**mm) must also
  be entered -- these can be found in Lin and Costello, and also Clark
  and Cain.

  The representation of the elements of GF(2**m) is either in index
  form, where the number is the power of the primitive element alpha,
  which is convenient for multiplication (add the powers
  modulo 2**m-1) or in polynomial form, where the bits represent the
  coefficients of the polynomial representation of the number, which
  is the most convenient form for addition.  The two forms are swapped
  between via lookup tables.  This leads to fairly messy looking
  expressions, but unfortunately, there is no easy alternative when
  working with Galois arithmetic.

  The code is not written in the most elegant way, but to the best of
  my knowledge, (no absolute guarantees!), it works.  However, when
  including it into a simulation program, you may want to do some
  conversion of global variables (used here because I am lazy!) to
  local variables where appropriate, and passing parameters (eg array
  addresses) to the functions  may be a sensible move to reduce the
  number of global variables and thus decrease the chance of a bug
  being introduced.

  This program does not handle erasures at present, but should not be
  hard to adapt to do this, as it is just an adjustment to the
  Berlekamp-Massey algorithm. It also does not attempt to decode past
  the BCH bound.
  -- see Blahut "Theory and practiceof error control codes"
	 for how to do this.

			  Simon Rockliff, University of Adelaide   21/9/89    }
			  */


#define mm 8				// { RS code over GF(2**mm) - change to suit }
#define nn  (1 << mm) - 1	// { nn=2**mm -1   length of codeword       }
#define MaxErrors 4        // { number of errors that can be corrected }
#define np 2 * MaxErrors	// { number of parity symbols               }
#define kk  nn - np         //{ data symbols, kk = nn-2*MaxErrors      }

			  /*
			  short = short ;

				  TReedSolomon = Class(TComponent)
								   Procedure generate_gf ;
								   Procedure gen_poly    ;

								   Procedure SetPrimitive(Var PP             ;
															  nIdx : Integer  ) ;

								 Public
								   Procedure InitBuffers ;

								   Procedure EncodeRS(Var xData    ;
													  Var xEncoded  ) ;
								   Function  DecodeRS(Var xData    ;
													  Var xDecoded  ) : Integer ;

								   Constructor Create(AOwner : TComponent) ; Reintroduce ;
								   Destructor Destroy ; Reintroduce ;
								 End ;
			  */
			  // specify irreducible polynomial coeffts }

Byte PP[17];

Byte CodeWord[256];
short Original_Recd[256];

short bb[np];
short data[256]; // 
short recd[nn];

short alpha_to[nn + 1];
short index_of[nn + 1];
short gg[np + 1];

string cDuring;
string cName;


//aPPType = Array[2..16] of Pointer;

void *  pPP[17];

Byte PP2[] = { 1 , 1 , 1 };

//   { 1 + x + x^3 }

Byte PP3[] = { 1 , 1 , 0 , 1 };

//     { 1 + x + x^4 }
Byte PP4[] = { 1 , 1 , 0 , 0 , 1 };

//     { 1 + x^2 + x^5 }
Byte PP5[] = { 1 , 0 , 1 , 0 , 0 , 1 };

//     { 1 + x + x^6 }
Byte PP6[] = { 1 , 1 , 0 , 0 , 0 , 0 , 1 };

//     { 1 + x^3 + x^7 }
Byte PP7[] = { 1, 0, 0, 1, 0, 0, 0, 1 };

//     { 1+x^2+x^3+x^4+x^8 }
Byte PP8[] = { 1 , 0 , 1 , 1 , 1 , 0 , 0 , 0 , 1 };

//     { 1+x^4+x^9 }
Byte PP9[] = { 1, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

//    { 1+x^3+x^10 }
Byte PP10[] = { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1 };

//     { 1+x^2+x^11 }
Byte PP11[] = { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

//     { 1+x+x^4+x^6+x^12 }
Byte PP12[] = { 1, 1, 0, 0, 1, 0, 1, 0, 0,
	0, 0, 0, 1 };

//    { 1+x+x^3+x^4+x^13 }
Byte PP13[] = { 1, 1, 0, 1, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 1 };

//     { 1+x+x^6+x^10+x^14 }
Byte PP14[] = { 1, 1, 0, 0, 0, 0, 1, 0, 0,
	0, 1, 0, 0, 0, 1 };

//     { 1+x+x^15 }
Byte PP15[] = { 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1 };
//     { 1+x+x^3+x^12+x^16 }
Byte PP16[] = { 1, 1, 0, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 0, 0, 0, 1 };


void InitBuffers();

/***********************************************************************
*                                                                      *
*       TReedSolomon.SetPrimitive                                      *
*                                                                      *
*         Primitive polynomials - see Lin & Costello, Appendix A,      *
*       and  Lee & Messerschmitt, p. 453.                              *
*                                                                      *
*       Modifications                                                  *
*       =============                                                  *
*                                                                      *
***********************************************************************/

void SetPrimitive(void* PP, int nIdx)
{
	move(pPP[nIdx], PP, (nIdx + 1));
}

/************************************************************************
*                                                                      *
*       Generate_GF                                                    *
*                                                                      *
*       Modifications                                                  *
*       =============                                                  *
*                                                                      *
***********************************************************************/

void Generate_gf()
{
	/* generate GF(2**mm) from the irreducible polynomial p(X)
	  in pp[0]..pp[mm]

	  lookup tables:
		  index->polynomial form   alpha_to[] contains j=alpha**i ;
		  polynomial form -> index form  index_of[j=alpha**i] = i
		  alpha = 2 is the primitive element of GF(2**mm)
	*/

	int i;
	short mask;

	SetPrimitive(PP, mm);

	mask = 1;
	alpha_to[mm] = 0;
	for (i = 0; i < mm; i++)
	{
		alpha_to[i] = mask;
		index_of[alpha_to[i]] = i;

		if (PP[i] != 0)
			alpha_to[mm]  = alpha_to[mm] ^ mask;
		mask = mask << 1;
	}


	index_of[alpha_to[mm]] = mm;
	mask = mask >> 1;

	for (i = mm + 1; i < nn; i++)
	{
		if (alpha_to[i - 1] >= mask)
			alpha_to[i] = alpha_to[mm] ^ ((alpha_to[i - 1] ^ mask) << 1);
		else
			alpha_to[i] = alpha_to[i - 1] << 1;

		index_of[alpha_to[i]] = i;
	}
	index_of[0] = -1;
}


/***********************************************************************
*                                                                      *
*       Gen_Poly                                                       *
*                                                                      *
*       Modifications                                                  *
*       =============                                                  *
*                                                                      *
***********************************************************************/

void gen_poly()
{
	/* Obtain the generator polynomial of the tt-error correcting, length
  nn=(2**mm -1) Reed Solomon code  from the product of
  (X+alpha**i), i=1..2*tt
*/

	short  i, j;

	gg[0] = 2;  //{ primitive element alpha = 2  for GF(2**mm) }
	gg[1] = 1;  //{ g(x) = (X+alpha) initially                 }

	i = nn;
	j = kk;

	j = i - j;

	for (i = 2; i <= 8; i++)
	{
		gg[i] = 1;

		for (j = (i - 1); j > 0; j--)
		{
			if (gg[j] != 0)
				gg[j] = gg[j - 1] ^ alpha_to[(index_of[gg[j]] + i) % nn];
			else
				gg[j] = gg[j - 1];
		}

		// { gg[0] can never be zero }

		gg[0] = alpha_to[(index_of[gg[0]] + i) % nn];
	}

	// { Convert gg[] to index form for quicker encoding. }

	for (i = 0; i <= np; i++)
		gg[i] = index_of[gg[i]];

}


/***********************************************************************
*                                                                      *
*       TxBase.Create                                                  *
*                                                                      *
*       Modifications                                                  *
*       =============                                                  *
*                                                                      *
***********************************************************************/

void RsCreate()
{
	InitBuffers();

	//{ generate the Galois Field GF(2**mm) }
	Generate_gf();
	gen_poly();
}

/***********************************************************************
*                                                                      *
*       TReedSolomon.Destroy                                           *
*                                                                      *
*       Modifications                                                  *
*       =============                                                  *
*                                                                      *
************************************************************************/

/***********************************************************************
*                                                                      *
*       TReedSolomon.EncodeRS                                          *
*                                                                      *
*       Modifications                                                  *
*       =============                                                  *
*                                                                      *
***********************************************************************/

void EncodeRS(Byte * xData, Byte * xEncoded)
{

	/* take the string of symbols in data[i], i=0..(k-1) and encode
	  systematically to produce 2*tt parity symbols in bb[0]..bb[2*tt-1]

	  data[] is input and bb[] is output in polynomial form.  Encoding is
	  done by using a feedback shift register with appropriate connections
	  specified by the elements of gg[], which was generated above.

	  Codeword is   c(X) = data(X)*X**(np)+ b(X) }
	  */

	  //	Type
	  //		bArrType = Array[0..16383] of Byte;

	int nI, i, j;

	short feedback;

	// absolute means variables share the same data

	//axData : bArrType absolute xData ;


	memset(bb, 0, sizeof(bb));

	for (nI = 0; nI < nn; nI++)
		data[nI] = xData[nI];

	for (i = (kk - 1); i >= 0; i--)
	{
		feedback = index_of[data[i] ^ bb[np - 1]];

		if (feedback != -1)
		{
			for (j = (np - 1); j > 0; j--)
			{
				if (gg[j] != -1)
					bb[j] = bb[j - 1] ^ alpha_to[(gg[j] + feedback) % nn];
				else
					bb[j] = bb[j - 1];
			}
			bb[0] = alpha_to[(gg[0] + feedback) % nn];
		}
		else
		{
			for (j = (np - 1); j > 0; j--)
				bb[j] = bb[j - 1];
			bb[0] = 0;
		}
	}

	//{ put the transmitted codeword, made up of data }
	//{ plus parity, in CodeWord                      }

	for (nI = 0; nI < np; nI++)
		recd[nI] = bb[nI];

	for (nI = 0; nI < kk; nI++)
		recd[nI + np] = data[nI];

	for (nI = 0; nI < nn; nI++)
		CodeWord[nI] = recd[nI];

	move(CodeWord, xEncoded, nn);
}



/***********************************************************************
*                                                                      *
*       DecodeRS                                                       *
*                                                                      *
*       Modifications                                                  *
*       =============                                                  *
*                                                                      *
***********************************************************************}

Function TReedSolomon.DecodeRS(Var xData    ;
							   Var xDecoded  ) : Integer ;
{ assume we have received bits grouped into mm-bit symbols in recd[i],
  i=0..(nn-1),  and recd[i] is index form (ie as powers of alpha).
  We first compute the 2*tt syndromes by substituting alpha**i into
  rec(X) and evaluating, storing the syndromes in s[i], i=1..2tt
  (leave s[0] zero).  Then we use the Berlekamp iteration to find the
  error location polynomial elp[i].   If the degree of the elp is >tt,
  we cannot correct all the errors and hence just put out the information
  symbols uncorrected. If the degree of elp is <=tt, we substitute
  alpha**i , i=1..n into the elp to get the roots, hence the inverse
  roots, the error location numbers. If the number of errors located
  does not equal the degree of the elp, we have more than tt errors and
  cannot correct them.  Otherwise, we then solve for the error value at
  the error location and correct the error.  The procedure is that found
  in Lin and Costello. for the cases where the number of errors is known
  to be too large to correct, the information symbols as received are
  output (the advantage of systematic encoding is that hopefully some
  of the information symbols will be okay and that if we are in luck, the
  errors are in the parity part of the transmitted codeword).  Of course,
  these insoluble cases can be returned as error flags to the calling
  routine if desired. */


int DecodeRS(Byte * xData, Byte * xDecoded)
{
	UNUSED(xDecoded);

//	string	cStr;
	int 	nI; // , nJ, nK;

	int 	i, j;
//	short 	u, q;

//	short elp[np + 2][np];
//	short d[np + 2];
//	short l[np + 2];
//	short u_lu[np + 2];
	short s[np + 1];

//	short	count;
	short	syn_error;

//	short root[MaxErrors];
//	short loc[MaxErrors];
//	short z[MaxErrors];
//	short err[nn];
//	short reg[MaxErrors + 1];

	for (nI = 0; nI < nn; nI++)
		recd[nI] = xData[nI];

	for (i = 0; i < nn; i++)
		recd[i] = index_of[recd[i]];  // { put recd[i] into index form }

//	count = 0;
	syn_error = 0;

	//	{ first form the syndromes }

	for (i = 0; i < np; i++)
	{
		s[i] = 0;

		for (j = 0; j < nn; j++)
		{
			if (recd[j] != -1)
				//	{ recd[j] in index form }
			{
				s[i] = s[i] ^ alpha_to[(recd[j] + i * j) % nn];
			}
		}

		//{ convert syndrome from polynomial form to index form  }
		if (s[i] != 0)
		{
			syn_error = 1; // { set flag if non-zero syndrome => error }
		}
		s[i] = index_of[s[i]];
	}

	if (syn_error != 0)  // { if errors, try and correct }
	{
		/*
				{ Compute the error location polynomial via the Berlekamp      }
				{ iterative algorithm, following the terminology of Lin and    }
				{ Costello:   d[u] is the 'mu'th discrepancy, where u = 'mu' + 1  }
				{ and 'mu' (the Greek letter!) is the step number ranging from }
				{ -1 to 2 * tt(see L&C), l[u] is the degree of the elp at that }
				{ step, and u_l[u] is the difference between the step number   }
				{and the degree of the elp.                                    }

				{ Initialize table entries }
				d[0]      : = 0; { index form      }
				d[1]      : = s[1]; { index form      }
				elp[0][0] : = 0; { index form      }
				elp[1][0] : = 1; { polynomial form }

				for i : = 1 to(np - 1) do
					Begin
					elp[0][i] : = -1; { index form      }
				elp[1][i] : = 0; { polynomial form }
				End;

				l[0]    : = 0;
				l[1]    : = 0;
				u_lu[0] : = -1;
				u_lu[1] : = 0;
			u: = 0;

				While((u < np) and (l[u + 1] <= MaxErrors)) do
					Begin
					Inc(u);

				If(d[u] = -1) then
					Begin
					l[u + 1] : = l[u];

				for i : = 0 to l[u] do
					Begin
					elp[u + 1][i] : = elp[u][i];
				elp[u][i]   : = index_of[elp[u][i]];
				End;
				End
					Else
				{ search for words with greatest u_lu[q] for which d[q] != 0 }
					Begin
					q : = u - 1;

				While((d[q] = -1) and (q > 0)) do
					Dec(q);

				{ have found first non - zero d[q] }
				If(q > 0) then
					Begin
					j : = q;

				While j > 0 do
					Begin
					Dec(j);
				If((d[j] != -1) and (u_lu[q] < u_lu[j])) then
					q : = j;
				End;
				End;

				{ have now found q such that d[u] != 0 and u_lu[q] is maximum }
				{ store degree of new elp polynomial                        }
				If(l[u] > l[q] + u - q) then
					l[u + 1] : = l[u]
					Else
					l[u + 1] : = l[q] + u - q;

				{ form new elp(x) }
				for i : = 0 to(np - 1) do
					elp[u + 1][i] : = 0;

				for i : = 0 to l[q] do
					If(elp[q][i] != -1) then
					elp[u + 1][i + u - q] : = alpha_to[(d[u] + nn - d[q] + elp[q][i]) mod nn];

				for i : = 0 to l[u] do
					Begin
					elp[u + 1][i] : = elp[u + 1][i] ^ elp[u][i];
				{ convert old elp value to index }
				elp[u][i]   : = index_of[elp[u][i]];
				End;
				End;

				u_lu[u + 1] : = u - l[u + 1];

				{ form(u + 1)th discrepancy }
				If u < np then{ no discrepancy computed on last iteration }
					Begin
					If(s[u + 1] != -1) then
					d[u + 1] : = alpha_to[s[u + 1]]
					Else
					d[u + 1] : = 0;

				for i : = 1 to l[u + 1] do
					If((s[u + 1 - i] != -1) and (elp[u + 1][i] != 0)) then
					d[u + 1] : = d[u + 1] ^ alpha_to[(s[u + 1 - i] + index_of[elp[u + 1][i]]) mod nn];
				{ put d[u + 1] into index form }
				d[u + 1] : = index_of[d[u + 1]];
				End;
				End; { end While }

				Inc(u);
				If l[u] <= MaxErrors then{ can correct error }
					Begin
				{ put elp into index form }
					for i : = 0 to l[u]do
						elp[u][i] : = index_of[elp[u][i]];

				{ find roots of the error location polynomial }
				for i : = 1 to l[u] do
					Begin
					reg[i] : = elp[u][i];
				End;

				for i : = 1 to nn do
					Begin
					q : = 1;

				for j : = 1 to l[u] do
					If reg[j] != -1 then
					Begin
					reg[j] : = (reg[j] + j) mod nn;
			q: = q ^ alpha_to[reg[j]];
				End;

				If q = 0 then{ store root and error location number indices }
					Begin
					root[count] : = i;
				loc[count] : = nn - i;
				Inc(count);
				End;
				End;

				If count = l[u] then{ no.roots = degree of elp hence <= tt errors }
					Begin
					Result : = count;

				{ form polynomial z(x) }
				for i : = 1 to l[u] do { Z[0] = 1 always - do not need }
				Begin
					If((s[i] != -1) and (elp[u][i] != -1)) then
					z[i] : = alpha_to[s[i]] ^ alpha_to[elp[u][i]]
					Else
					If((s[i] != -1) and (elp[u][i] = -1)) then
					z[i] : = alpha_to[s[i]]
					Else
					If((s[i] = -1) and (elp[u][i] != -1)) then
					z[i] : = alpha_to[elp[u][i]]
					Else
					z[i] : = 0;

				for j : = 1 to(i - 1) do
					if ((s[j] != -1) and (elp[u][i - j] != -1)) then
						z[i] : = z[i] ^ alpha_to[(elp[u][i - j] + s[j]) mod nn];

				{ put into index form }
				z[i] : = index_of[z[i]];
				End;

				{ evaluate errors at locations given by }
				{ error location numbers loc[i]         }
				for i : = 0 to(nn - 1) do
					Begin
					err[i] : = 0;
				If recd[i] != -1 then{ convert recd[] to polynomial form }
					recd[i] : = alpha_to[recd[i]]
					Else
					recd[i] : = 0;
				End;

				for i : = 0 to(l[u] - 1) do { compute numerator of error term first }
				Begin
					err[loc[i]] : = 1; { accounts for z[0] }
				for j : = 1 to l[u] do
					If z[j] != -1 then
					err[loc[i]] : = err[loc[i]] ^ alpha_to[(z[j] + j * root[i]) mod nn];

				If err[loc[i]] != 0 then
					Begin
					err[loc[i]] : = index_of[err[loc[i]]];

			q: = 0; { form denominator of error term }

				for j : = 0 to(l[u] - 1) do
					If j != i then
					q : = q + index_of[1 ^ alpha_to[(loc[j] + root[i]) mod nn]];
			q: = q mod nn;

				err[loc[i]] : = alpha_to[(err[loc[i]] - q + nn) mod nn];
				{ recd[i] must be in polynomial form }
				recd[loc[i]] : = recd[loc[i]] ^ err[loc[i]];
				End;
				End;
				End
					Else{ no.roots != degree of elp = > > tt errors and cannot solve }
					Begin
					Result : = -1; { Signal an error. }

				for i : = 0 to(nn - 1) do { could return error flag if desired }
				If recd[i] != -1 then{ convert recd[] to polynomial form }
					recd[i] : = alpha_to[recd[i]]
					Else
					recd[i] : = 0; { just output received codeword as is }
				End;
				End{ if l[u] <= tt then }
					Else{ elp has degree has degree > tt hence cannot solve }
					for i : = 0 to(nn - 1) do { could return error flag if desired }
				If recd[i] != -1 then{ convert recd[] to polynomial form }
					recd[i] : = alpha_to[recd[i]]
					Else
					recd[i] : = 0; { just output received codeword as is }
				End{ If syn_error != 0 then }
				{ no non - zero syndromes = > no errors : output received codeword }
				Else
					Begin
					for i : = 0 to(nn - 1) do
						If recd[i] != -1 then{ convert recd[] to polynomial form }
						recd[i] : = alpha_to[recd[i]]
						Else
						recd[i] : = 0;

			Result: = 0; { No errors ocurred. }
				End;

				for nI : = 0 to(NN - 1) do
					axDecoded[nI] : = Recd[nI];
				End; { TReedSolomon.DecodeRS }
				*/

		return syn_error;
	}
	return 0;
}



/***********************************************************************
*                                                                      *
*       TReedSolomon.InitBuffers                                       *
*                                                                      *
*       Modifications                                                  *
*       =============                                                  *
*                                                                      *
***********************************************************************/

void InitBuffers()
{
	memset(data, 0, sizeof(data));
	memset(recd, 0, sizeof(recd));
	memset(CodeWord, 0, sizeof(CodeWord));

	//{ Initialize the Primitive Polynomial vector. }
	pPP[2] = PP2;
	pPP[3] = PP3;
	pPP[4] = PP4;
	pPP[5] = PP5;
	pPP[6] = PP6;
	pPP[7] = PP7;
	pPP[8] = PP8;
	pPP[9] = PP9;
	pPP[10] = PP10;
	pPP[11] = PP11;
	pPP[12] = PP12;
	pPP[13] = PP13;
	pPP[14] = PP14;
	pPP[15] = PP15;
	pPP[16] = PP16;
}

