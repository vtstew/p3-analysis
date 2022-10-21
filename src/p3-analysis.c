/**
 * @file p3-analysis.c
 * @brief Compiler phase 3: static analysis
 * @name Vivian Stewart and Katie Brasacchio
 */
#include "p3-analysis.h"

/**
 * @brief State/data for static analysis visitor
 */
typedef struct AnalysisData
{
    /**
     * @brief List of errors detected
     */
    ErrorList *errors;
    bool is_loop;
    bool is_return;
    DecafType funcdecl_return_type;

    /* BOILERPLATE: TODO: add any new desired state information (and clean it up in AnalysisData_free) */

} AnalysisData;

/**
 * @brief Allocate memory for analysis data
 *
 * @returns Pointer to allocated structure
 */
AnalysisData *AnalysisData_new()
{
    AnalysisData *data = (AnalysisData *)calloc(1, sizeof(AnalysisData));
    CHECK_MALLOC_PTR(data);
    data->errors = ErrorList_new();
    return data;
}

/**
 * @brief Deallocate memory for analysis data
 *
 * @param data Pointer to the structure to be deallocated
 */
void AnalysisData_free(AnalysisData *data)
{
    /* free everything in data that is allocated on the heap except the error
     * list; it needs to be returned after the analysis is complete */

    /* free "data" itself */
    free(data);
}

/**
 * @brief Macro for more convenient access to the data inside a @ref AnalysisVisitor
 * data structure
 */
#define DATA ((AnalysisData *)visitor->data)

/**
 * @brief Macro for more convenient access to the error list inside a
 * @ref AnalysisVisitor data structure
 */
#define ERROR_LIST (((AnalysisData *)visitor->data)->errors)

/**
 * @brief Wrapper for @ref lookup_symbol that reports an error if the symbol isn't found
 *
 * @param visitor Visitor with the error list for reporting
 * @param node AST node to begin the search at
 * @param name Name of symbol to find
 * @returns The @ref Symbol if found, otherwise @c NULL
 */
Symbol *lookup_symbol_with_reporting(NodeVisitor *visitor, ASTNode *node, const char *name)
{
    Symbol *symbol = lookup_symbol(node, name);
    if (symbol == NULL)
    {
        ErrorList_printf(ERROR_LIST, "Symbol '%s' undefined on line %d", name, node->source_line);
    }
    return symbol;
}

/**
 * @brief Macro for shorter storing of the inferred @c type attribute
 */
#define SET_INFERRED_TYPE(T) ASTNode_set_printable_attribute(node, "type", (void *)(T), \
                                                             type_attr_print, dummy_free)

/**
 * @brief Macro for shorter retrieval of the inferred @c type attribute
 */
#define GET_INFERRED_TYPE(N) (DecafType) ASTNode_get_attribute(N, "type")

/**
 * @brief check if there is a main method.
 *
 * @param visitor
 * @param node
 */
void AnalysisVisitor_check_main_fun(NodeVisitor *visitor, ASTNode *node)
{
    if (lookup_symbol(node, "main") == NULL)
    {
        Error_throw_printf("Program does not contain a 'main' function");
    }
}

/**
 * @brief check for void vardecls.
 *
 * @param visitor
 * @param node
 */
void AnalysisVisitor_check_vardecl(NodeVisitor *visitor, ASTNode *node)
{
    if (node->vardecl.type == VOID)
    {
        Error_throw_printf("Void variable '%s' on line %d", node->vardecl.name, node->source_line);
    }

    if (node->vardecl.is_array)
    {
        if (node->vardecl.array_length <= 0)
        {
            Error_throw_printf("Array length must be greater than 0");
        }
    }
}

/**
 * @brief check location symbol.
 *
 * @param visitor
 * @param node
 */
void AnalysisVisitor_check_location(NodeVisitor *visitor, ASTNode *node)
{
    if (DATA->is_return)
    {
        Symbol *sym = lookup_symbol(node, node->location.name);
        
        if (sym != NULL && sym->type != DATA->funcdecl_return_type)
        {
            Error_throw_printf("Expected %s return type but type was %s\n", DecafType_to_string(DATA->funcdecl_return_type), DecafType_to_string(sym->type));
        }
    }
    lookup_symbol_with_reporting(visitor, node, node->location.name); //== NULL
}

/**
 * @brief set the infered type to the node literal type.
 *
 * @param visitor
 * @param node
 */
void Analysis_literal_infer(NodeVisitor *visitor, ASTNode *node)
{
    SET_INFERRED_TYPE(node->literal.type);
}

void Analysis_previsit_while_loop(NodeVisitor *visitor, ASTNode *node)
{
    DATA->is_loop = true;
}

void Analysis_postvisit_while_loop(NodeVisitor *visitor, ASTNode *node)
{
    DATA->is_loop = false;
}

void Analysis_previsit_break(NodeVisitor *visitor, ASTNode *node)
{
    if (!DATA->is_loop) {
        Error_throw_printf("Break statement should be inside a while loop.");
    }
}

void Analysis_previsit_continue(NodeVisitor *visitor, ASTNode *node)
{
    if (!DATA->is_loop) {
        Error_throw_printf("Continue statement should be inside a while loop.");
    }   
}

void Analysis_previsit_funcdecl(NodeVisitor *visitor, ASTNode *node)
{
    DATA->funcdecl_return_type = node->funcdecl.return_type;
}

void Analysis_previsit_return(NodeVisitor *visitor, ASTNode *node)
{
    DATA->is_return = true;
    if (node->funcreturn.value->type == LOCATION)
    {
        // do nothing and let previsit_location handle error

    } else if (node->funcreturn.value->type == LITERAL)
    {
        if (node->funcreturn.value->literal.type != DATA->funcdecl_return_type)
        {
            Error_throw_printf("Expected %s return type but type was %s\n", DecafType_to_string(DATA->funcdecl_return_type), DecafType_to_string(node->funcreturn.value->literal.type) );
        }
    }
}

void Analysis_postvisit_return(NodeVisitor *visitor, ASTNode *node)
{
    DATA->is_return = false;
}


ErrorList *analyze(ASTNode *tree)
{
    /* allocate analysis structures */
    NodeVisitor *v = NodeVisitor_new();
    v->data = (void *)AnalysisData_new();
    v->dtor = (Destructor)AnalysisData_free;

    /* BOILERPLATE: TODO: register analysis callbacks */
    v->previsit_vardecl = AnalysisVisitor_check_vardecl;
    v->previsit_program = AnalysisVisitor_check_main_fun;
    v->previsit_location = AnalysisVisitor_check_location;
    v->previsit_literal = Analysis_literal_infer;
    v->previsit_whileloop = Analysis_previsit_while_loop;
    v->previsit_break = Analysis_previsit_break;
    v->previsit_funcdecl = Analysis_previsit_funcdecl;
    v->previsit_return = Analysis_previsit_return;
    v->postvisit_return = Analysis_postvisit_return;
    
    

    // assign a tyoe to all the literals according to chart thing

    /* perform analysis, save error list, clean up, and return errors */
    NodeVisitor_traverse(v, tree);
    ErrorList *errors = ((AnalysisData *)v->data)->errors;
    NodeVisitor_free(v);
    return errors;
}
