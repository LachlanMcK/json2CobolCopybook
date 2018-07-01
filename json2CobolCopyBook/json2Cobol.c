#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

#include "json2Cobol.h"

#include "json2Cobol.h"

//extern "C"
//int main()
//{
//	int a; //This allocates an integer on the stack
//	int* pA = &a;  //This allocates an int pointer on the stack and sets its value to point to a.The '*' is part of the declaration.
//	pA = &a; //Now this is the same as the second statement. It sets the value of pA to point to a.
//}

//This is used to compare whether two strings (property names in this case) are equal
int myStrCmp(char * s1, char * s2) {
	unsigned i;
	//if one string is zero length then both must be to be equal
	if (strlen(s1) == 0)
		if (strlen(s2) == 0)
			return 0;
		else
			return -1;

	for (i = 0; i < strlen(s1) && i < strlen(s2); i++)
		if (s1[i] != s2[i])
			return -1;
	return i;
}

json_type  myType(jsonWanted e) {
	if (e.type[0] == 'a') return JSON_ARRAY;
	if (e.type[0] == 'o') return JSON_OBJECT;
	if (e.type[0] == 's') return JSON_STRING;
	if (e.type[0] == 'n') return JSON_NUMBER;
	if (e.type[0] == 'b') return JSON_PRIMITIVE;
	return JSON_UNDEFINED;
}


MYAPI int parse(const char * inputJson, const int count, const char * deiredStructure, char * output )
{
	json_parser *parser = (json_parser *)malloc(SIZE_OF_JSON_PARSER);
	parser->pos = 0;

	int sss = sizeof( json_parser);
	json_parser *p = malloc( sss);

	jsonWanted *wantedStructure;
	
	//allocates memory for an array of jsonWanted to hold the "desiredStructure" input.  This is freed below.
	wantedStructure = (jsonWanted*) malloc(SIZE_OF_JSONWanted * (count+1));

	//allocates memory for the "parsed" json.  This will be realloc'ed if not big enough.  It is freed below.
	parser->numberAllNodesAllocated = count * 2;
	parser->allNodes = (parsedJson*) malloc(SIZE_OF_PARSEDJSON * parser->numberAllNodesAllocated); //allocate with pleanty to spare

	parser->allNodesIndex = -1;
	createWantedStructure(&count, wantedStructure, deiredStructure);

	//parse the input json String into the "parsedJson" structure
	int result = doParse(parser, inputJson);
	if (result < 0)
		return result;

	result = doMove(parser, parser->allNodes[0],  count,  wantedStructure, (char*) output	);

	free(wantedStructure);
	free(parser->allNodes);
	free(parser);

	return result;
}

void createWantedStructure(const int *count, jsonWanted * wantedStructure, const char * deiredStructure)
{
	//the zero'th wanted position is reserved for the JSON root object
	wantedStructure[0].firstMatchNodeIndex= wantedStructure[0].parentNodeIndex =-1;
	wantedStructure[0].length = 0;
	wantedStructure[0].name  = wantedStructure[0].arrayCountLen = wantedStructure[0].arrayMaxLen = '\0';
	wantedStructure[0].type = NULL;

	for (int i = 1;i <= *count;i++) {
		wantedStructure[i].name = (char*)deiredStructure;
		deiredStructure = deiredStructure + strlen(deiredStructure) + 1;
		wantedStructure[i].type = (char*)deiredStructure;
		deiredStructure = deiredStructure + strlen(deiredStructure) + 1;
		wantedStructure[i].length = (char*)deiredStructure;
		deiredStructure = deiredStructure + strlen(deiredStructure) + 1;
		if (wantedStructure[i].type[0] == 'a') {
			wantedStructure[i].arrayCountLen = (char*)deiredStructure;
			deiredStructure = deiredStructure + strlen(deiredStructure) + 1;
			wantedStructure[i].arrayMaxLen = (char*)deiredStructure;
			deiredStructure = deiredStructure + strlen(deiredStructure) + 1;
		} else 
			wantedStructure[i].arrayMaxLen = wantedStructure[i].arrayCountLen = (char *) "";
			
		wantedStructure[i].parentNodeIndex = strlen(deiredStructure) == 0? -1: strtol((char*)deiredStructure, NULL, 10);
		deiredStructure = deiredStructure + strlen(deiredStructure) + 1;

		wantedStructure[i].firstMatchNode = NULL;
		wantedStructure[i].firstMatchNodeIndex = -1;
	};
}

 MYAPI int print_line(const char* str) {
	printf("%s\n", str);
	return 6;
 }

 //each found node in the json string has one of these tokens created.
 //given we don't know how many there is going to be, the memory for this may need to be realloced
 parsedJson* allocNewToken(json_parser *parser, int parent, int predecessor, json_type type) {

	parser->allNodesIndex++;
	if (parser->numberAllNodesAllocated-1 < parser->allNodesIndex) {
		parser->numberAllNodesAllocated = parser->numberAllNodesAllocated * 2;
		parsedJson *tmp = (parsedJson *) realloc(parser->allNodes, SIZE_OF_PARSEDJSON * parser->numberAllNodesAllocated);
		if (!tmp) {
			free(parser->allNodes);
			return tmp;
		} else
			parser->allNodes = tmp;
	}
	
	parsedJson * result = &parser->allNodes[parser->allNodesIndex];
	result->parent = parent;
	result->prev = predecessor;
	result->next = -1;

	result->type = type;
	result->thisElementsIndex = parser->allNodesIndex;
	
	result->skippedItemsCount = 0;
	result->start = result->end = result->size = 0;
	result->arrayIndex = 0;
	if (parent != -1 && predecessor != -1) 
		if (parser->allNodes[parent].type == JSON_ARRAY)
			result->arrayIndex = parser->allNodes[predecessor].arrayIndex + 1;
	result->propertyName = (char *) "";
	result->value = (char *) "";
	result->outputNext = NULL;
	
	result->wanted = NULL;
	result->spotForCount = NULL;
	result->arrayOwner = NULL;
	return result;
 }

 /*	This function goes through the input JSON string character by character and creates a structure (held in parser->allNodes) which
	has pointers into the inputJson
 */
 int doParse(json_parser* parser, const char *inputJson) {
	unsigned len = strlen(inputJson);
	char c;
	bool obj_arry_hasName = false;

	json_type type;
	const char true_primative[6] = "true";
	const char false_primative[6] = "false";
	const char null_primative[6] = "NULL";
	const char empty[2] = " ";

	parsedJson *current = NULL, *root = &parser->allNodes[0];
	int parent = -1, predecessor = -1;

	for (; parser->pos < len && inputJson[parser->pos] != '\0'; parser->pos++) {

		c = inputJson[parser->pos];
		switch (c) {
		case '{': case '[':
			obj_arry_hasName = false;

			type = (c == '{' ? JSON_OBJECT : JSON_ARRAY);

			if (current == NULL)													//first time
				current = allocNewToken(parser, parent, predecessor, type);
			else
				if (current->type == JSON_PROPERTY)									//this object or array already has a name
					obj_arry_hasName = true;
				else
					current = allocNewToken(parser, parent, predecessor, type);		//anonymous object/array

			parent = current->thisElementsIndex;

			current->start = parser->pos;											//don't yet know the size or end
			if (obj_arry_hasName) 
				current->type = type;												//JSON_PROPERTY means we've already seen the property so now we must be up to the object/array after the :
			else {	
				current->propertyName = (char *) "";
				current->value = (char *) "";
			}

			predecessor = -1;														//starting a new object or array, so the first property wont have a predecessor

			break;

		case '}': case ']':
			type = (c == '}' ? JSON_OBJECT : JSON_ARRAY);

			if (current->parent) {													//if there is a parent, then "pop" it back to be current
				current = &parser->allNodes[ parent];
				parent = current->parent;
				current->end = parser->pos;
				current->size = current->end - current->start + 1;
				predecessor = current->prev;
			}

			break;

		case '\"':
			
			if (current->type == JSON_PROPERTY)
				current->type = JSON_STRING;										//JSON_PROPERTY means we've already seen the property so now we must be up to the string after the :
			else if (current->type == JSON_ARRAY)
				current = allocNewToken(parser, current->thisElementsIndex, predecessor, JSON_STRING);
			else
				current = allocNewToken(parser, parent, predecessor, JSON_PROPERTY);//otherwise we're dealing with a property name

			current->start = parser->pos + 1;										//add one more for the leading "
			for (parser->pos++; parser->pos < len && inputJson[parser->pos] != '\"'; parser->pos++) { //jump to the end of the string

			}
			current->end = parser->pos - 1;											//take one off for the trailing "
			current->size = parser->pos - current->start;

			if (current->type == JSON_PROPERTY)
				current->propertyName = (char *)inputJson + current->start;			//point into the json string (no need to copy it)

			if (current->type == JSON_STRING)										//ditto
				current->value = (char *)inputJson + current->start;

			break;

		case ' ':case '\n':case '\f':case '\t':case '\r':							//basically ignore any white space
			type = JSON_UNDEFINED;
			break;

		case ':':
			type = JSON_UNDEFINED;
			break;

		case ',':
			predecessor = current->thisElementsIndex;
			parser->allNodes[predecessor].next = parser->allNodesIndex + 1;			//the next will be the next found node
			current = &parser->allNodes[parent];
			break;

		case '0': case '1': case '2': case '3':case '4': case '5':case '6': case '7':case '8': case '9': case '.':

			if (current->type == JSON_PROPERTY)
				current->type = JSON_NUMBER;										//JSON_PROPERTY means we're dealing with a NUMBER after the :
			else if (current->type == JSON_ARRAY)
				current = allocNewToken(parser, current->thisElementsIndex, predecessor, JSON_NUMBER);
			else
				return -3;															//numbers can't be anoymous (except in arrays)

			current->start = parser->pos;

			for (parser->pos++; parser->pos < len; parser->pos++) {
				if (inputJson[parser->pos] == '0' || inputJson[parser->pos] == '1' || inputJson[parser->pos] == '2' || inputJson[parser->pos] == '3' || inputJson[parser->pos] == '4' || inputJson[parser->pos] == '5' || inputJson[parser->pos] == '6' || inputJson[parser->pos] == '7' || inputJson[parser->pos] == '8' || inputJson[parser->pos] == '9' || inputJson[parser->pos] == '.')
					continue;
				else
					break;
			}
			parser->pos--;
			current->end = parser->pos;
			current->size = parser->pos + 1 - current->start;

			current->value = (char *)inputJson + current->start;
			break;

		case 'f': case 't': case 'N':

			current->start = parser->pos;

			if (current->type == JSON_PROPERTY)
				current->type = JSON_PRIMITIVE;										//JSON_PROPERTY means we're dealing with a NUMBER after the :
			else if (current->type == JSON_ARRAY)
				current = allocNewToken(parser, parent, predecessor, JSON_PRIMITIVE);
			else
				return -4;

			current->size = -1;
			if ((myStrCmp((char *)true_primative, (char *)&inputJson[parser->pos]) == 4)) {
				current->end = current->start + 3;
				current->size = 4;
				current->value = (char *)inputJson + current->start;
			}
			if ((myStrCmp((char *)false_primative, (char *)&inputJson[parser->pos]) == 5)) {
					current->end = current->start + 4;
					current->size = 5;
					current->value = (char *)inputJson + current->start;
			}
			if ((myStrCmp((char *)null_primative, (char *)&inputJson[parser->pos]) == 4)) {
					current->end = current->start + 3;
					current->size = 4;

			}
			parser->pos = current->end;
			break;
			
		default:
			return -5;

		} //switch
	} //for loop
	return 0;
 }



 // this function  finds the nodes in the parsed JSON that correspond to the wanted nodes so that the content may be returned
 // each wanted node is contextualised by its parent.
 // this function just does the firest step, find the wanted nodes
 errorCode matchParsedJsonToWantedStructure(json_parser * parser, jsonWanted * wantedStructure, int i)
 {

	//special rules for the first wanted node because that is the root of the json	 
	if (i == 0) {
	wantedStructure[0].firstMatchNode = &parser->allNodes[0];
		wantedStructure[0].firstMatchNodeIndex = 0;
		return OK;
	}

	parsedJson * current = &parser->allNodes[0];
	parsedJson * parentNode = NULL;
	int wantedParentNodeIndex = wantedStructure[i].parentNodeIndex;  
	jsonWanted * wantedParent = wantedParentNodeIndex <= 0? NULL : &wantedStructure[wantedParentNodeIndex];

	errorCode found = matchNotFound;

	for (int j = wantedParentNodeIndex; j < parser->allNodesIndex + 1; j++) {
	//for (int j = 1; j < parser->allNodesIndex + 1; j++) {
		current = &parser->allNodes[j];

		if (wantedParentNodeIndex < 0) continue;
		if (current->parent < 0) continue;

		parentNode = &parser->allNodes[current->parent];
		
		//I never try to match position 0
		//If wantedParentNodeIndex is > 0 and its not wanted, then the children won't be wanted.
		if (parentNode->wanted == NULL && wantedParentNodeIndex > 0)
			continue;

		if (myStrCmp(wantedStructure[i].name, current->propertyName) == strlen(wantedStructure[i].name) &&
			(wantedParent == parentNode->wanted ) &&
			myType(wantedStructure[i]) == current->type) {

			found = OK;
			if (current->wanted != NULL)
				continue;

			if (wantedStructure[i].firstMatchNode == NULL) {
				wantedStructure[i].firstMatchNode = &parser->allNodes[j];
				wantedStructure[i].firstMatchNodeIndex = j;
			}

			current->wanted = &wantedStructure[i];

			if (parentNode->arrayOwner == NULL && parentNode->type != JSON_ARRAY)
				return OK;
			else
				if (parentNode->arrayOwner != NULL)
					current->arrayOwner = parentNode->arrayOwner;
				else				
					if (parentNode->type == JSON_ARRAY) 
						current->arrayOwner = parentNode;

			//this is an optimisation, should be able to stop looking for wanted nodes after the end of the top level array parent
			if (current->parent == ((parsedJson *) current->arrayOwner)->thisElementsIndex && current->next == -1)
				break;
		};
	}

	return found;
 }

 errorCode createDepthFirstPath(json_parser * parser) {
	parsedJson * current = &parser->allNodes[0];
	parsedJson *parentNode = NULL, *prevNode = NULL;


	for (int j = 1; j < parser->allNodesIndex + 1; j++) {
		current = &parser->allNodes[j];
		jsonWanted * wanted = (jsonWanted *)current->wanted;
		if (current->parent == -1)
			continue;

		parentNode = &parser->allNodes[current->parent];
		if (current->prev == -1) {
			parentNode->outputNext = current;
			prevNode = NULL;
		} else
			prevNode = &parser->allNodes[current->prev];

		//if there is a next, then that becomes the outputNext (this may later change if current is an object or array)
		if (current->next != -1) {
			current->outputNext = &parser->allNodes[current->next];
		} else {
			//if there is no next, then the outputNext becomes the next of some ancestor which has a next

			while (parentNode != NULL)
				if (parentNode->next != -1) {
					current->outputNext = &parser->allNodes[parentNode->next];
					break;
				} else
					if (parentNode->parent != -1)
						parentNode = &parser->allNodes[parentNode->parent];
					else
						break;
		}

	}
	return OK;
 }

 //this follows the depth first path to extract the content of each wanted node
 errorCode extractValuesFromParsedJson2(json_parser * parser, parsedJson * current, char * output, int * len)
 {
	 *len = 0;
	jsonWanted *parentWanted = NULL, * wanted = (jsonWanted *)current->wanted;
	if (wanted == NULL) return OK;

	errorCode result = extractFailed, doingArrayItem = false;
	int  arrayMaxLen = -1, arrayCountLen;
	char * tmpOutput = output;
	
	if (current->parent != -1)
		if (parser->allNodes[current->parent].type == JSON_ARRAY) {
			doingArrayItem = true;
			arrayMaxLen = parser->allNodes[current->parent].arrayMaxLen;
			parentWanted = (jsonWanted*)parser->allNodes[current->parent].wanted;
			arrayCountLen = strtol(parentWanted->arrayCountLen, NULL, 10);
			//because arrays can have multiple types, only return the wanted types
			if (myType(*wanted) != current->type) {
				*len = 0;
				parser->allNodes[current->parent].skippedItemsCount++;
				return OK;
			}
		}

	int parentNodeIndex = -1;
	*len = strtol(wanted->length, NULL, 10);

	if (wanted->firstMatchNode->type == JSON_ARRAY) {
		//remember this for after the array has been processed
		current->arrayMaxLen = strtol(wanted->arrayMaxLen, NULL, 10);

		parentNodeIndex = wanted->firstMatchNodeIndex;

		if (myStrCmp(wanted->type, (char *) "ab") == 2) {
			current->spotForCount = (char *)tmpOutput;
			arrayCountLen = strtol(wanted->arrayCountLen, NULL, 10);
			tmpOutput = tmpOutput + arrayCountLen;
			*len = arrayCountLen;
		}
		if (myStrCmp(wanted->type, (char *) "aa") == 2) {
			current->spotForCount = tmpOutput + current->arrayMaxLen * *len;
			*len = 0;
		}
		result = OK;
	}

	if (current->type == JSON_OBJECT) {
		result = OK;
	}
	if (current->type == JSON_STRING) {

		for (int k = 0; k < current->size; k++)
			tmpOutput[k] = current->value[k];

		for (int k = current->size; k < *len; k++)
			*((char *)(tmpOutput + k)) = ' ';

		tmpOutput = &tmpOutput[*len];
		result = OK;
	}
	if (current->type == JSON_NUMBER) {
		int padding = *len - current->size;
		if (padding < 0) return lengthTooShort;

		for (int k = 0; k < padding; k++)
			tmpOutput[k] = '0';

		for (int k = padding; k < padding + current->size; k++)
			tmpOutput[k] = current->value[k - padding];

		tmpOutput = &tmpOutput[*len];
		result = OK;
	}
	if (current->type == JSON_PRIMITIVE) {
		if (current->size > 0) {
			if (current->value[0] == 't') tmpOutput[0] = '1';
			if (current->value[0] == 'f') tmpOutput[0] = '0';
			if (current->value[0] == 'N') tmpOutput[0] = '\0';
		}
		for (int k = 1; k < *len; k++)
			tmpOutput[k] = '\0';

		tmpOutput = &tmpOutput[*len];
		result = OK;
	}

	//see if we've had enough of doing the array and so have to now output the count
	bool exceededMax = true, nothingLeft=false;
	if (doingArrayItem  ) {
		int currentArrayIndex = current->arrayIndex + 1 - parser->allNodes[current->parent].skippedItemsCount;
		exceededMax = (currentArrayIndex >= arrayMaxLen);
		if (exceededMax)
			while (current->next != -1)
				*current = parser->allNodes[current->next];
			
		if (current->next == -1)
			nothingLeft = true;

		if (exceededMax || nothingLeft)	{
			char strCnt[12] = { 0 };
			sprintf_s(strCnt, 12, "%0*d",  arrayCountLen, currentArrayIndex); //the currentArrayIndex is the hightest indexed element - minus any skipped items
			for (int k = 0; k < arrayCountLen; k++) {
				parser->allNodes[current->parent].spotForCount[k] = strCnt[k];
			}
		}
		if (nothingLeft && parentWanted->type[1] == 'a') {
			*len = *len + (arrayMaxLen - currentArrayIndex) * *len + arrayCountLen;
		}
	}
	return result;
 }

 int doMove(json_parser * parser, const parsedJson parsedInput, const int count, jsonWanted * wantedStructure, char * output) {

	 for (int i = 0; i <= count; i++) {
		 if (matchParsedJsonToWantedStructure(parser, wantedStructure, i) != OK)
			 return -10;
	 };

	 if (createDepthFirstPath(parser) != OK)
		 return -11;

	 int len = 0;
	 char *tmpOut = output;

	 for (parsedJson *tmp = wantedStructure[0].firstMatchNode; tmp != NULL; tmp = tmp->outputNext) {
		 if (extractValuesFromParsedJson2(parser, tmp, tmpOut, &len) != OK)
			 return -12;
		 tmpOut = &tmpOut[len];
	 }

	 return 2;
 }
