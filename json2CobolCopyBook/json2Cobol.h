#ifndef json2CopyBook_h_
#define json2CopyBook_h_

#ifndef MYAPI
#define MYAPI 
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

	/**
	* JSON type identifier. Basic types are:
	* 	o Object
	* 	o Array
	*   o Property
	* 	o String
	* 	o Other primitive: number, boolean (true/false) or null
	*/
	typedef enum {
		JSON_UNDEFINED = 0,
		JSON_OBJECT = 1,
		JSON_ARRAY = 2,
		JSON_PROPERTY = 3,
		JSON_STRING = 4,
		JSON_PRIMITIVE = 5,
		JSON_NUMBER = 6
	} json_type;

	typedef enum {
		OK = 0,
		matchNotFound = -10,
		extractFailed =  -20,
		lengthTooShort

	} errorCode;

	typedef enum { false, true } bool;

	/**
	* JSON token description.
	* thisElementsIndex - where it was found in the JSON string
	* type		type (object, array, string etc.)

	* next, prev, parent - index of these related nodes
	
	* propertyName/ valuestring  - pointers to the JSON content
	
	* start, end, size	- char offsets in JSON data string

	* arrayIndex (where in the array this node was found)
	* arrayOwner (pointer back to the containing array node)

	* a bunch of details about the array nodes:
	* skippedItemsCount (how many are ignored)
	* spotForCount - point to output byte array where the array count should be positioned
	* arrayMaxLen - how many items in the array to output

	* wanted - pointer to the desired wantedStructure that means this node will be output

	* outputNext - pointer to the next node in a depth first walk of nodes
	*/
	typedef struct parsedJson
	{
		int thisElementsIndex;
		/* The type of the item, as above. */
		json_type type;

		/* after we find all the nodes, we set the "outputNext" so we can do a "depth first" walk of the JSON */
		void * outputNext;

		/* next/prev allow you to walk array/object chains. */
		int next;
		int prev;
		int parent;
		

		/* The propertyName from the JSON */
		char *propertyName;

		/* The item's VALUE, if type==JSON_STRING, JSON_Number, JSON_PRIMITIVE  */
		char *value;
		
		/* the start, end posy in the input json */
		int start;
		int end;
		int size;

		/* if this node is part of an array, this is its array position */
		int arrayIndex;
		void * arrayOwner;

		/* not everything in an array may be "desired", this keeps a count of the number of items skipped so can produce an accurate count of items in output */
		int skippedItemsCount;

		char * spotForCount;
		int arrayMaxLen;
		

		/* if we decide we want this node, we set a pointer to the jsonWanted structure that identifies it is wanted */
		void * wanted;
	} parsedJson;

	const SIZE_OF_PARSEDJSON = sizeof(parsedJson);

	/**
	* JSON parser. Contains an array of token blocks available. Also stores
	* the string being parsed now and current position in that string
	* pos - where we're up to in reading the JSON string
	* allNodes - an array of parsedNodes
	* allNodesIndex - the index of the highest identified node
	* numberAllNodesAllocated - a count of how many array items have memory allocagted
	*/
	typedef struct {
		unsigned int pos; /* offset in the JSON string */
		parsedJson * allNodes;
		int allNodesIndex;
		int numberAllNodesAllocated;
	} json_parser;

	const SIZE_OF_JSON_PARSER = sizeof(json_parser);

   /*
	* this holds details about wanted nodes
	* name: the propertyName of the node wanted (in context of parentNode Index below)
	* type: the type of node wanded
	* length: how many bytes of output this node will need to occupy
	* for arrays...
	* arrayCountLen: arrays have an item count - this says how many bytes that count should occupy
	* arraymaxLen: what is the most number of nodes that should be output
	* parentNodeIndex: the index to another wanted node that is the parent of this one
	* firstMatchNode: a point to the first node that satisfies this wanted node (there may be others)
	* firstMatchNodeIndex: an index to the parser->allNodes for the first node that matches this wanted node
  */
	typedef struct {
		char *name;
		char *type;
		char *length;
		char *arrayCountLen;
		char *arrayMaxLen;
		int  parentNodeIndex;
		parsedJson * firstMatchNode;
		int  firstMatchNodeIndex;
	} jsonWanted;

	const SIZE_OF_JSONWanted = sizeof(jsonWanted);

	MYAPI int print_line(const char* str);

   /*	parse
	*	----------------------------
	*	This is the main function in this program.
	*	
	*	It takes a string representing some well formed JSON, plus a series of null terminated strings which describe what is wanted out of that JSON
	*	and returns the wanted contents as a byte array.
	*
	*	inputs:
	*	-------
	*   inputJson:	a null terminated string representign well formed JSON - behaviour is undefined if not well formed
	*   count:		count of the number of "wanted" items - this count must be right or else incorrect results will occur
	*	desired:	a string representing what elements of the JSON to return.  This string is broken down as follows:
	*
	*				For non-array nodes:
	*				name -	the JSON property name to match on
	*				type -	the type of JSON property to look for 
	*						s - string
	*						n - number
	*						b - primitive element (true, false, NULL)
	*				length - how long to pad the returned value out to
	*						strings - padded with trailing spaces (truncated if shorter than actual value
	*						numbers - padded with leading zeros (errors if shorter than actual value
	*						primitives - padded with trailing nulls
	*				parent node index - this is the node that this desired is a child of
	*
	*				For array nodes:
	*				name -	the JSON property name to match on
	*				type -	the type of JSON property to look for
	*						ab - array (count before),
	*						aa - array (count after),
	*
	*						arrays always return a count of the number of items taken from the JSON string
	*						
	*				array Count Length - how long to pad array count with leading zeros
	*				array max length - how many elements the array may contain
	*						excess elements are ignored
	*				parent node index - this is the node that this desired is a child of
	*
	*	outputs:
	*	-------
	*	output:	the byte array that represents the concatenated desired contents from the JSON string
	*			the invoker should allocate this byte array with adequate length to hold the result
	*
	*   returns: 0		in the case of success
	*			-1		notused
	*			-2		notused
	*			-3		the thing before a number is not a propertyName or is not part of an array
	*			-4		the thing before a primitive is not a propertyName or is not part of an array
	*			-5		something unexpected in the JSON
	*			-10		couldn't find an wanted node
	*			-11		something went wrong producing the depth first path
	*			-12		something went wrong extracting the values for the output
	*
	*/

	MYAPI int parse(const char * inputJson, const int count, const char * desired, char * output);
	
	int doParse(json_parser * parser, const char * inputJson);

	//void createWantedStructure(const int &count, jsonWanted * wantedStructure, const char * &deiredStructure);
	void createWantedStructure(const int * count, jsonWanted * wantedStructure, const char * deiredStructure);
	
	parsedJson* allocNewToken(json_parser *parser, int parent, int predecessor, json_type type);
	
	errorCode matchParsedJsonToWantedStructure(json_parser * parser, jsonWanted * wantedStructure, int i);

	errorCode createDepthFirstPath(json_parser * parser);

	//errorCode  extractValuesFromParsedJson2(json_parser * parser, parsedJson current, char * &output, int &len);
	errorCode  extractValuesFromParsedJson2(json_parser * parser, parsedJson * current, char * output, int * len);

	int doMove(json_parser * parser, const parsedJson parsedInput, const int count, jsonWanted * wantedStructure, char * output);

#ifdef __cplusplus
}
#endif

#endif