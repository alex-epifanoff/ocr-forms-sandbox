#include "parser.h"
#include <iostream>
#include <assert.h>

using namespace std;

static int jsoneq(const char *json, const jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

static string tokstr( const string& json, const jsmntok_t& tok )
{
    assert(tok.type==JSMN_STRING);    
    return json.substr( tok.start, tok.end-tok.start );
}

static float tokfloat( const string& json, const jsmntok_t& tok )
{
    assert(tok.type==JSMN_PRIMITIVE || tok.type==JSMN_STRING);
    return atof( json.substr( tok.start, tok.end-tok.start ).c_str() );
}

static int tokint( const string& json, const jsmntok_t& tok )
{
    assert(tok.type==JSMN_PRIMITIVE || tok.type==JSMN_STRING);
    return atoi( json.substr( tok.start, tok.end-tok.start ).c_str() );
}

static int parsePath( const string& json_data, const jsmntok_t* tokens, const int pos, vector<cv::Vec2f>& path)
{
    int ntok = pos+1;

    assert(tokens[pos].type==JSMN_ARRAY);

    for ( int i = 0; i < tokens[pos].size; ++i ) {
        cv::Vec2f p;

        assert(tokens[ntok].type==JSMN_ARRAY);
        assert(tokens[ntok].size==2);
        assert(tokens[ntok+1].type==JSMN_PRIMITIVE);
        assert(tokens[ntok+2].type==JSMN_PRIMITIVE);

        p[0] = tokfloat(json_data, tokens[++ntok]);
        p[1] = tokfloat(json_data, tokens[++ntok]);
        ++ntok;

        path.push_back( p );
    }

    return ntok;
}

static int parseArea( const string& json_data, const jsmntok_t* tokens, const int pos, DocTemplate::Area& area)
{
    int ntok = pos+1;

    assert(tokens[pos].type==JSMN_OBJECT);
    assert(tokens[pos].size==4);

    for ( int i = 0; i < tokens[pos].size; ++i ) {
        assert(tokens[ntok].type==JSMN_STRING);

        if (jsoneq(json_data.c_str(), &tokens[ntok], "name") == 0) {
            ++ntok;
            area.name = tokstr(json_data,tokens[ntok]);
            ++ntok;            
        } else if (jsoneq(json_data.c_str(), &tokens[ntok], "type") == 0) {
            ++ntok;
            string sType = tokstr(json_data,tokens[ntok]);
            ++ntok;

            if (sType=="text") {
                area.type = DocTemplate::Area::arText;
            } else if (sType=="image") {
                area.type = DocTemplate::Area::arImage;
            } else {
                assert(false);
            }

        } else if (jsoneq(json_data.c_str(), &tokens[ntok], "path") == 0) {
            ++ntok;
            ntok = parsePath( json_data, tokens, ntok, area.path );

        } else if (jsoneq(json_data.c_str(), &tokens[ntok], "angle") == 0) {
            ++ntok;
            area.angle = tokint(json_data,tokens[ntok]);
            ++ntok;
        } else {
            assert(false);
        }
    }
    return ntok;
}


static int parseAreas( const string& json_data, const jsmntok_t* tokens, const int pos, vector<DocTemplate::Area>& areas )
{    
    int ntok = pos+1;

    assert(tokens[pos].type==JSMN_ARRAY);

    for ( int i = 0; i < tokens[pos].size; ++i ) {
        DocTemplate::Area area;
        ntok = parseArea( json_data, tokens, ntok, area );
        areas.push_back(area);
    }

    return ntok;
}

int readTemplate( const char* filename, DocTemplate& docTemp )
{
    ifstream infile;
    infile.open(filename);

    string json_data;

    infile >> json_data;

    jsmn_parser parser;
    jsmntok_t tokens[4096];

    jsmn_init( &parser );

	int r = jsmn_parse(&parser, json_data.c_str(), json_data.size(), tokens, sizeof(tokens)/sizeof(tokens[0]));
	if (r < 0)
    {
		cerr << "Failed to parse JSON: " << r << endl;
		return 1;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || tokens[0].type != JSMN_OBJECT)
    {
		printf("Object expected\n");
		return 1;
	}

	for (int i = 1; i < r; i++)
    {
		if (jsoneq(json_data.c_str(), &tokens[i], "aspect") == 0)
        {
            ++i;
            docTemp.aspect = tokfloat( json_data, tokens[i] ); 

        }
        else if (jsoneq(json_data.c_str(), &tokens[i], "areas") == 0 )
        {            
            ++i;            
            i = parseAreas( json_data, tokens, i, docTemp.areas );            
        }
	}

    return 0;
}