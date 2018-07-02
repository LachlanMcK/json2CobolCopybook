json2Cobol extracts the data contents from a JSON string into a flat byte array.  
- It takes a (wellformed) json string and 
- Another string which contains a bunch of null terminated strings which are the instructions of what to pull out from the
json string.
- It takes a count of the number of null terminated strings.

- It returns a byte array of the contents of the JSON, padded to fixed field format suitable for cobol copybook.

If the JSON string or the instructions of what to extract are not well formed, the results will be unpredictable.