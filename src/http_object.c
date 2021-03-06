#include<http_object.h>

char charToHex(char c)
{
	if( '0' <= c && c <= '9' )
		return c - '0';
	else if('a' <= c && c <= 'f')
		return c - 'a' + 10;
	else if('A' <= c && c <= 'F')
		return c - 'A' + 10;
	else
		return 'N';
}

void urlToString(char* path_param_str)
{
	char* ptemp = path_param_str;
	char* update = ptemp;
	while(*ptemp != '\0')
	{
		if(*ptemp == '%')
		{
			*update = (( charToHex(*(ptemp+1)) << 4 ) | charToHex(*(ptemp+2)));
			ptemp+=2;
		}
		else
			*update = *ptemp;
		update++;
		ptemp++;
	}
	*update = '\0';
}

char hexToChar(char hex)
{
	hex = hex & 0x0f;
	if( 0 <= hex && hex <= 9 )
		return hex + '0';
	return (hex - 10) + 'a';
}

int characterAllowedInURL(char c)
{
	if( ('0'<=c && c<='9') || ('a'<=c && c<='z') || ('A'<=c && c<='Z') )
		return 1;
	switch(c)
	{
		case '$' :	case '-' :	case '_' :	case '.' :
		case '+' :	case '!' :	case '*' :	case '\'' :
		case '(' :	case ')' :	case ',' :
			return 1;
		default :
			return 0;
	}
}

static void serialize_paramter_helper(dstring* result, const dstring* input)
{
	char temp[10];
	for(int i=0; i<input->bytes_occupied; i++)
	{
		if( characterAllowedInURL(input->cstring[i]) )
		{
			temp[0] = input->cstring[i];
			temp[1] = '\0';
		}
		else
		{
			temp[0] = '%';
			temp[1] = hexToChar((input->cstring[i] >> 4) & 0x0f);
			temp[2] = hexToChar(input->cstring[i] & 0x0f);
			temp[3] = '\0';
		}
		concatenate_dstring(result, dstring_DUMMY_CSTRING(temp));
	}
}

void serialize_parameter_entry(const dstring* key, const dstring* value, dstring* result)
{
	serialize_paramter_helper(result, key);
	concatenate_dstring(result, dstring_DUMMY_CSTRING("="));
	serialize_paramter_helper(result, value);
	concatenate_dstring(result, dstring_DUMMY_CSTRING("&"));
}

void serialize_header_entry(const dstring* key, const dstring* value, dstring* result)
{
	concatenate_dstring(result, key);
	concatenate_dstring(result, dstring_DUMMY_CSTRING(": "));
	concatenate_dstring(result, value);
	concatenate_dstring(result, dstring_DUMMY_CSTRING("\r\n"));
}

void print_entry_wrapper(const dstring* key, const dstring* value, const void* addpar)
{
	printf("\t\t[");
	printf_dstring(key);
	printf("]->[");
	printf_dstring(value);
	printf("]\n");
}

int hasHeader(char* Key, char* Value, dmap* headers)
{
	dstring* value_test = (dstring*) find_equals_in_dmap_cstr(headers, Key);

	if(value_test == NULL)
		return 0;

	return (compare_dstring(value_test, dstring_DUMMY_CSTRING(Value)) == 0);
}
