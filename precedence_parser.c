#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef uint64_t u64;
typedef int64_t  s64;

typedef u64 Node_ID;

typedef struct {
//{{{ 
        char *p;
        u64   n;
//}}}
} String;

typedef enum {
        Token_Type_Invalid,
        Token_Type_Number,
        Token_Type_Binary_Operator,
        Token_Type_EOF,
        Token_Type_Done,     // EOF is generated once after that Done is generated
                             // from here on these are skip tokens
        Token_Type_Skip,     // separation space
        Token_Type_Space,    // separation space
        Token_Type_Comment,  // anythin starting with a '#' to a new line
} Token_Type;

typedef enum {
        Node_Type_Leaf,
        Node_Type_Binary_Operator
} Node_Type;

typedef enum {
        Binary_Operator_Invalid,
        Binary_Operator_Add,
        Binary_Operator_Subtract,
        Binary_Operator_Multiply,
        Binary_Operator_Divide,
        Binary_Operator_Greater,
        Binary_Operator_Less,
} Binary_Operator;

static int binary_operator_precedence[] = {
        [Binary_Operator_Add]      = 2,
        [Binary_Operator_Subtract] = 2,

        [Binary_Operator_Multiply] = 3,
        [Binary_Operator_Divide]   = 3,

        [Binary_Operator_Greater]  = 1,
        [Binary_Operator_Less]     = 1,
};

typedef struct {
//{{{ 
        u64             offset;
        u64             length;
        Token_Type      type:16;
        Binary_Operator binary_operator:16; // only valid if type is Token_Type_Binary_Operator
//}}}
} Token;

typedef struct {
//{{{ 
        Node_Type  type;

        Token      token;

        Node_ID    left;
        Node_ID    right;
//}}}
} Node;

typedef struct {
//{{{ 
        char *data;
        u64   offset;
        u64   length;
        bool  done;
//}}}
} Stream;

bool is_a_space_character(char c)
//{{{ 
{
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}
//}}}

bool is_a_digit_character(char c)
//{{{ 
{
        return c >= '0' && c <= '9';
}
//}}}

bool is_a_binary_op_character(char c)
//{{{ 
{
        return (c == '+' || c == '-' || c == '*' || c == '/' || c == '>' || c == '<');
}
//}}}

Binary_Operator binary_op_char_to_type(char c)
//{{{ 
{
        switch (c) {
                case '+': return Binary_Operator_Add;
                case '-': return Binary_Operator_Subtract;
                case '*': return Binary_Operator_Multiply;
                case '/': return Binary_Operator_Divide;
                case '>': return Binary_Operator_Greater;
                case '<': return Binary_Operator_Less;
                default:  return Binary_Operator_Invalid;
        }
}
//}}}

Token next_token(Stream *stream)
//{{{
{
        if (stream->done) {
                //{{{
                return (Token) {
                        .type = Token_Type_Done,
                };
                //}}}
        }

        char *begin = stream->data + stream->offset;
        char *end   = stream->data + stream->length;

        char *it    = begin;

        Token tk = {
                .length = 0,
                .offset = stream->offset,
                .type = Token_Type_Invalid,
        };

        // could return separator tokens
        if (it == end) {
                //{{{ 
                stream->done = true;
                tk.type = Token_Type_EOF;
                //}}}
        } else if (is_a_space_character(*it)) {
                //{{{ 
                ++it;
                while (it < end && is_a_space_character(*it)) {
                        ++it;
                }
                tk.length = it - begin;
                tk.type = Token_Type_Space;
                stream->offset += tk.length;
                //}}}
        } else if (*it == '#') {
                //{{{ 
                ++it;
                while (it < end && *it != '\n') {
                        it++;
                }
                tk.length = it - begin;
                tk.type = Token_Type_Comment;
                stream->offset += tk.length;
                //}}}
        } else if (is_a_digit_character(*it)) {
                //{{{ 
                ++it;
                while (it < end && is_a_digit_character(*it)) {
                        ++it;
                }
                tk.length = it - begin;
                tk.type = Token_Type_Number;
                stream->offset += tk.length;
                //}}}
        } else if (is_a_binary_op_character(*it)) {
                //{{{
                Binary_Operator op = binary_op_char_to_type(*it);
                ++it;
                tk.length = it - begin;
                tk.type = Token_Type_Binary_Operator;
                tk.binary_operator = op;
                stream->offset += tk.length;
                //}}}
        } else {
                //{{{ 
                tk.length = end - begin;
                tk.type = Token_Type_Invalid;
                stream->offset += tk.length;
                //}}}
        }
        return tk;
}
//}}}

Token next_token_non_skip_token(Stream *stream)
//{{{
{
        for (;;) {
                Token tk = next_token(stream);
                if (tk.type >= Token_Type_Skip) {
                        continue;
                }
                return tk;
        }
}
//}}}

typedef struct {
        //{{{
        struct {
                Node *p;
                u64   n;
                u64   c;
        } nodes;
        u64      num_nodes;
        u64      capacity;
        Node_ID  root;
        Stream  *stream;
        struct {
                Token head; // there is only zero or one token in the buffer
                u64   n;
        } non_skip_tokens;
        //}}}
} Parser;

Token parser_get_current_non_skip_token(Parser *self)
//{{{ 
{
        if (self->non_skip_tokens.n == 0) {
                self->non_skip_tokens.head = next_token_non_skip_token(self->stream);
                self->non_skip_tokens.n = 1;
        }
        return self->non_skip_tokens.head;
}
//}}}

void parser_consume_current_token(Parser *self)
//{{{ 
{
        assert(self->non_skip_tokens.n > 0);
        self->non_skip_tokens.n = 0;
}
//}}}

Node *parser_new_node(Parser *self)
//{{{ 
{
        if (self->nodes.n == self->nodes.c) {
                u64 new_capacity = self->nodes.c * 2;
                if (new_capacity < 16) {
                        new_capacity = 16;
                }
                self->nodes.p = realloc(self->nodes.p, new_capacity * sizeof(Node));
                self->nodes.c = new_capacity;
        }
        Node *node = self->nodes.p + self->nodes.n;
        *node = (Node) { 0 };
        ++self->nodes.n;
        return node;
}
//}}}

String parser_token_string(Parser *self, Token tk)
//{{{ 
{
        return (String) {
                .p = self->stream->data + tk.offset,
                .n = tk.length,
        };
}
//}}}

Node_ID parse_leaf(Parser *self);
Node_ID parse_expression(Parser *self, int min_precedence);
Node_ID parse_increasing_precedence(Parser *self, Node_ID left, int min_precedence);

Node_ID parse_leaf(Parser *self)
//{{{ 
{
        Token tk = parser_get_current_non_skip_token(self);
        if (tk.type != Token_Type_Number) {
                return 0;
        }
        parser_consume_current_token(self);

        Node *node = parser_new_node(self);
        *node = (Node) {
                .type = Node_Type_Leaf,
                .token = tk,
        };

        Node_ID id = node - self->nodes.p;
        return id;
}
//}}}

Node_ID parse_increasing_precedence(Parser *self, Node_ID left, int min_precedence)
//{{{ 
{
        Token tk = parser_get_current_non_skip_token(self);
        if (tk.type != Token_Type_Binary_Operator) {
                return left;
        }

        int next_precedence = binary_operator_precedence[tk.binary_operator];

        if (next_precedence <= min_precedence) {

                return left;

        } else {

                parser_consume_current_token(self);

                Node_ID right = parse_expression(self, next_precedence);

                if (right == 0) {
                        Token tk = parser_get_current_non_skip_token(self);
                        printf("Error: expected a number at offset %llu  %.*s\n", tk.offset, (int)tk.length, self->stream->data + tk.offset);
                        return 0; // dummy node is a sign of error
                }

                Node *node = parser_new_node(self);
                *node = (Node) {
                        .type  = Node_Type_Binary_Operator,
                        .token = tk,
                        .left  = left,
                        .right = right,
                };
                Node_ID id = node - self->nodes.p;
                return id;
        }
}
//}}}

Node_ID parse_expression(Parser *self, int min_precedence)
//{{{ 
{
        Node_ID left = parse_leaf(self);
        if (left == 0) {
                Token tk = parser_get_current_non_skip_token(self);
                printf("Error: expected a number at offset %llu\n", tk.offset);
                return 0; // dummy node is a sign of error
        }
        for(;;) {
                Node_ID node = parse_increasing_precedence(self, left, min_precedence);
                if (node == left) {
                        break;
                }
                left = node;
        }
        return left;
}
//}}}

Parser parse(Stream *stream)
//{{{ 
{
        Parser parser = {
                .stream = stream,
        };

        // create a dummy node (node 0)
        parser_new_node(&parser);

        parser.root = parse_expression(&parser, 0); // parse with the min precedence

        Token tk = parser_get_current_non_skip_token(&parser);
        if (tk.type != Token_Type_EOF) {
                printf("Error: expected EOF at offset %llu\n", tk.offset);
                parser.root = 0; // dummy node is a sign of error
        }

        return parser;
}

//}}}

//
// parse expression in a smart way with operator precedence
// as Jon Blow showed in his video
//
// 2 * 3 + 5 * 4 - 6 > 7 * 8
//

void parser_print_node(Parser *self, Node_ID node_id, int level)
{
        Node *node = self->nodes.p + node_id;
        if (node->type == Node_Type_Leaf) {
                String tk_str = parser_token_string(self, node->token);
                printf("%*sLeaf: %.*s\n", 
                                4*level, "", 
                                (int) tk_str.n, tk_str.p);
        } else {
                String tk_str = parser_token_string(self, node->token);
                printf("%*sBinary Operator: %.*s\n", 
                                4*level, "",
                                (int) tk_str.n, tk_str.p);
                parser_print_node(self, node->left,  level+1);
                parser_print_node(self, node->right, level+1);
        }
}

void parser_print_tree(Parser *self)
//{{{ 
{
        parser_print_node(self, self->root, 0);
}
//}}}


int main() 
//{{{ 
{
        // char *expression = "((((2 * 3) + (5 * 4)) - 6) > (7 * 8))";

        char *expression = "2 * 3 + 5 * 4 - 6 > 7 * 8";

        Stream stream = {
                .data   = expression,
                .offset = 0,
                .length = strlen(expression),
        };

        Parser parser = parse(&stream);

        parser_print_tree(&parser);

        if (parser.root == 0) {
                printf("Error: expected a valid root\n");
        } else {
                printf("OK\n");
        }

        // print the tree
        return 1;
}
//}}}
