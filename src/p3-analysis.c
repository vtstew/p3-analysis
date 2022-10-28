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
    bool is_block;
    bool is_func;
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
 * @brief helper method for checking for duplicate symbols
 *
 * @param node
 */
void find_ducplicat_helper(ASTNode *node)
{
    // get the current symbol table
    SymbolTable *table = (SymbolTable *)ASTNode_get_attribute(node, "symbolTable");
    int count = 0;

    // compare each item to every other item in the list to find duplicates
    FOR_EACH(Symbol *, s1, table->local_symbols)
    {
        count = 0;
        FOR_EACH(Symbol *, s2, table->local_symbols)
        {
            if (strcmp(s1->name, s2->name) == 0)
            {
                count = count + 1;

                // make sure it hasn't found itself
                if (count > 1)
                {
                    Error_throw_printf("Duplicate names %s\n", s1->name);
                }
            }
        }
    }
}

/**
 * @brief previsit program node
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_program(NodeVisitor *visitor, ASTNode *node)
{
    if (node == NULL)
    {
        ErrorList_printf(ERROR_LIST, "NULL Tree");
    }
    // make sure there is a "main" function
    else if (lookup_symbol(node, "main") == NULL)
    {
        ErrorList_printf(ERROR_LIST, "Program does not contain a 'main' function");
    }
    // make sure the thing called "main" is a function
    else if (lookup_symbol(node, "main")->symbol_type != FUNCTION_SYMBOL)
    {
        ErrorList_printf(ERROR_LIST, "Program does not contain a 'main' function");
    }
    // check for no paramaters in the main method
    else if (lookup_symbol(node, "main")->parameters->size > 0)
    {
        ErrorList_printf(ERROR_LIST, "'main' must take no parameters");
    }
    // check for multiple methods with the same name
    else
    {
        find_ducplicat_helper(node);
    }
}

/**
 * @brief postvisit program
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_program(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        // make sure the "main" method is not null
        if (lookup_symbol(node, "main") != NULL)
        {
            // make sure "main" returns an INT
            if (lookup_symbol(node, "main")->type != INT)
            {
                ErrorList_printf(ERROR_LIST, "Program 'main' function must return an int");
            }
        }
    }
}

/**
 * @brief previsit vardecl
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_vardecl(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        SET_INFERRED_TYPE(node->vardecl.type);
    }
}

/**
 * @brief postvisit vardecl
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_vardecl(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        DecafType type = GET_INFERRED_TYPE(node);
        // variable cannot be VOID type
        if (type == VOID)
        {
            ErrorList_printf(ERROR_LIST, "Void variable '%s' on line %d", node->vardecl.name, node->source_line);
        }
        else
        {
            // check for valid array size
            if (node->vardecl.is_array)
            {
                // array declarations can only be global
                if (DATA->is_block || DATA->is_func)
                {
                    ErrorList_printf(ERROR_LIST, "Local variable '%s' on line %d cannot be an array", node->vardecl.name, node->source_line);
                    return;
                }
                if (node->vardecl.array_length <= 0)
                {
                    ErrorList_printf(ERROR_LIST, "Array length must be greater than 0");
                }
            }
        }
    }
}

/**
 * @brief previsit funcdecl
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_funcdecl(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        SET_INFERRED_TYPE(node->funcdecl.return_type);
        // used to check that a function returns the correct type
        DATA->funcdecl_return_type = node->funcdecl.return_type;
        // used to check if array declaration happens in a function
        DATA->is_func = true;
    }
}

/**
 * @brief postvisit funcdecl
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_funcdecl(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        DATA->is_func = false;
        // check for duplicate variable declarations
        find_ducplicat_helper(node);
    }
}

/**
 * @brief previsit block
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_block(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        // used to check if array declarations happen in a block
        DATA->is_block = true;
    }
}

/**
 * @brief postvisit block
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_block(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        DATA->is_block = false;
        // check for duplicate variable declarations in a block
        find_ducplicat_helper(node);
    }
}

/**
 * @brief previsit assignment
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_assignment(NodeVisitor *visitor, ASTNode *node)
{
    // do nothing
}

/**
 * @brief postisit assignment
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_assignment(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        // get inferred types of left and right side fo assignment and make sure they're equal
        DecafType left_type = GET_INFERRED_TYPE(node->assignment.location);
        DecafType right_type = GET_INFERRED_TYPE(node->assignment.value);
        if (left_type != right_type)
        {
            ErrorList_printf(ERROR_LIST, "Expected %s type but type was %s", DecafType_to_string(left_type), DecafType_to_string(right_type));
        }
    }
}

/**
 * @brief previsit conditional
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_conditional(NodeVisitor *visitor, ASTNode *node)
{
}

/**
 * @brief postvisit conditional
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_conditional(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        DecafType cond_type = GET_INFERRED_TYPE(node->conditional.condition);
        // the condition must evaluate to a boolean
        if (cond_type != BOOL)
        {
            ErrorList_printf(ERROR_LIST, "Conditional type was %s, expected bool on line %d", DecafType_to_string(cond_type), node->source_line);
        }
    }
}

/**
 * @brief previsit while loop
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_while_loop(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        // used to check if break or continue happen inside a loop
        DATA->is_loop = true;
    }
}

/**
 * @brief postvisit while loop
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_while_loop(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        DATA->is_loop = false;
        DecafType cond_type = GET_INFERRED_TYPE(node->whileloop.condition);
        // condition must evaluate to a boolean
        if (cond_type != BOOL)
        {
            ErrorList_printf(ERROR_LIST, "Conditional type was %s, expected bool on line %d", DecafType_to_string(cond_type), node->source_line);
        }
    }
}

/**
 * @brief previsit return
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_return(NodeVisitor *visitor, ASTNode *node)
{
}

/**
 * @brief postvisit return
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_return(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL && node->funcreturn.value != NULL)
    {
        // if the function returns a variable, make sure it has been declared
        if (node->funcreturn.value->type == LOCATION)
        {
            if (lookup_symbol(node, node->funcreturn.value->location.name) == NULL)
            {
                return;
            }
        }

        // check that the function returns the correct type
        DecafType infer_return_value = GET_INFERRED_TYPE(node->funcreturn.value);
        if (infer_return_value != 0)
        {
            if (DATA->funcdecl_return_type != infer_return_value)
            {
                ErrorList_printf(ERROR_LIST, "Invalid return type, Expected %s was %s on line %d", DecafType_to_string(DATA->funcdecl_return_type), DecafType_to_string(infer_return_value), node->source_line);
            }
        }
    }
}

/**
 * @brief previsit break
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_break(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        // make sure breaks only happen inside a loop
        if (!DATA->is_loop)
        {
            ErrorList_printf(ERROR_LIST, "Break statement should be inside a while loop.");
        }
    }
}

/**
 * @brief postvisit break
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_break(NodeVisitor *visitor, ASTNode *node)
{
}

/**
 * @brief previsit continue
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_continue(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        // make sure continues only happen inside a loop
        if (!DATA->is_loop)
        {
            Error_throw_printf("Continue statement should be inside a while loop.");
        }
    }
}

/**
 * @brief postvisit continue
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_continue(NodeVisitor *visitor, ASTNode *node)
{
}

/**
 * @brief previsit binop
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_binop(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        BinaryOpType binop_type = node->binaryop.operator;
        // expressions with ||, &&, ==, !=, <, <=, >=, > evaluate to BOOL
        if (binop_type >= 0 && binop_type < 8)
        {
            SET_INFERRED_TYPE(BOOL);
            // expressions with +, -, *, /, % evaluate to INT
        }
        else
        {
            SET_INFERRED_TYPE(INT);
        }
    }
}

/**
 * @brief invisit binop
 *
 * @param visitor
 * @param node
 */
void Analysis_invisit_binop(NodeVisitor *visitor, ASTNode *node)
{
}

/**
 * @brief postvisit binop
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_binop(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        // get operater and inferred type of left and right side of the expression
        BinaryOpType binop_type = node->binaryop.operator;
        DecafType left_type = GET_INFERRED_TYPE(node->binaryop.left);
        DecafType right_type = GET_INFERRED_TYPE(node->binaryop.right);
        // expressions with <, <=, >=, >, +, -, *, /, % both sides must be INT
        if ((binop_type > 3) && (binop_type < 13))
        {
            if (left_type != INT || right_type != INT)
            {
                ErrorList_printf(ERROR_LIST, "Cannot use operator %s on type %s and %s on line %d", BinaryOpToString(binop_type), DecafType_to_string(left_type), DecafType_to_string(right_type), node->source_line);
                return;
            }
        }
        // expressions with == and != must have the same type on both sides
        else if ((binop_type == 2) || (binop_type == 3))
        {
            if (left_type != right_type)
            {
                ErrorList_printf(ERROR_LIST, "Cannot use operator %s on type %s and %s on line %d", BinaryOpToString(binop_type), DecafType_to_string(left_type), DecafType_to_string(right_type), node->source_line);
                return;
            }
        }
        // expressions with || and && must be BOOL on both side
        else if ((binop_type == 0) || (binop_type == 1))
        {
            if (left_type != BOOL || right_type != BOOL)
            {
                ErrorList_printf(ERROR_LIST, "Cannot use operator %s on type %s and %s on line %d", BinaryOpToString(binop_type), DecafType_to_string(left_type), DecafType_to_string(right_type), node->source_line);
                return;
            }
        }
    }
}

/**
 * @brief previsit unary operator
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_unop(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        UnaryOpType unop = node->unaryop.operator;
        // - operator evaluates to INT
        if (unop == 0)
        {
            SET_INFERRED_TYPE(INT);
        }
        // ! evaluates to BOOL
        else
        {
            SET_INFERRED_TYPE(BOOL);
        }
    }
}

/**
 * @brief postvisit unary operator
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_unop(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        // make sure inferred type of the child is correct
        DecafType actual_type = GET_INFERRED_TYPE(node->unaryop.child);
        DecafType inferred_type = GET_INFERRED_TYPE(node);
        if (actual_type != inferred_type)
        {
            ErrorList_printf(ERROR_LIST, "Type mismatch expected %s was %s on line %d", DecafType_to_string(inferred_type), DecafType_to_string(actual_type), node->source_line);
            return;
        }
    }
}

/**
 * @brief previsit location
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_location(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        // make sure variable exists in symbol table
        Symbol *sym = lookup_symbol_with_reporting(visitor, node, node->location.name);
        if (sym != NULL)
        {
            SET_INFERRED_TYPE(sym->type);
        }
    }
}

/**
 * @brief postvisit location
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_location(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        Symbol *sym = lookup_symbol(node, node->location.name);
        // check again if variable exists in symbol table
        if (sym != NULL)
        {
            // check for array location with no index
            if (sym->symbol_type == ARRAY_SYMBOL && node->location.index == NULL)
            {
                ErrorList_printf(ERROR_LIST, "Expected array index on line %d", node->source_line);
                return;
            }
            // check for array location with index that is not an INT
            if (sym->symbol_type == ARRAY_SYMBOL && GET_INFERRED_TYPE(node->location.index) != INT)
            {
                ErrorList_printf(ERROR_LIST, "Array index must be an integer on line %d", node->source_line);
                return;
            }
        }
    }
}

/**
 * @brief previsit function call
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_funcall(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        // make sure function exists in symbol table
        Symbol *sym = lookup_symbol_with_reporting(visitor, node, node->funccall.name);
        if (sym != NULL)
        {
            SET_INFERRED_TYPE(sym->type);
        }
    }
}

/**
 * @brief postvisit function call
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_funcall(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        Symbol *sym = lookup_symbol_with_reporting(visitor, node, node->funccall.name);
        // check again that function exists in symbol table
        if (sym != NULL)
        {
            // temporarily store head of parameter list
            struct Parameter *head = sym->parameters->head;
            // make sure there is the correct number of arguments
            if (sym->parameters->size != node->funccall.arguments->size)
            {
                ErrorList_printf(ERROR_LIST, "Incorrect number of arguments, expected %d, but got %d on line %d", sym->parameters->size, node->funccall.arguments->size, node->source_line);
                return;
            }
            // go through each parameter and make sure arguments are correct types
            for (int i = 0; i < sym->parameters->size; i++)
            {
                if (sym->parameters->head->type != GET_INFERRED_TYPE(node->funccall.arguments->head))
                {
                    ErrorList_printf(ERROR_LIST, "Expected type %s but got type %s on line %d", DecafType_to_string(sym->parameters->head->type), DecafType_to_string(GET_INFERRED_TYPE(node->funccall.arguments->head)), node->source_line);
                    return;
                }
                // move head to the next parameter
                sym->parameters->head = sym->parameters->head->next;
                // move head to the next argument
                node->funccall.arguments->head = node->funccall.arguments->head->next;
            }
            // set the head back
            sym->parameters->head = head;
        }
    }
}

/**
 * @brief previsit literal
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_literal(NodeVisitor *visitor, ASTNode *node)
{
    if (node != NULL)
    {
        SET_INFERRED_TYPE(node->literal.type);
    }
}

/**
 * @brief postvisit literal
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_literal(NodeVisitor *visitor, ASTNode *node)
{
}

ErrorList *analyze(ASTNode *tree)
{
    /* allocate analysis structures */
    NodeVisitor *v = NodeVisitor_new();
    v->data = (void *)AnalysisData_new();
    v->dtor = (Destructor)AnalysisData_free;

    // check if tree is null, if so create an error list and return
    if (tree == NULL)
    {
        ErrorList *errors = ((AnalysisData *)v->data)->errors;
        ErrorList_printf(errors, "Null Tree not allowed.");
        return errors;
    }

    /* BOILERPLATE: TODO: register analysis callbacks */
    v->previsit_program = Analysis_previsit_program;
    v->postvisit_program = Analysis_postvisit_program;

    v->previsit_vardecl = Analysis_previsit_vardecl;
    v->postvisit_vardecl = Analysis_postvisit_vardecl;

    v->previsit_funcdecl = Analysis_previsit_funcdecl;
    v->postvisit_funcdecl = Analysis_postvisit_funcdecl;

    v->previsit_block = Analysis_previsit_block;
    v->postvisit_block = Analysis_postvisit_block;

    v->previsit_assignment = Analysis_previsit_assignment;
    v->postvisit_assignment = Analysis_postvisit_assignment;

    v->previsit_conditional = Analysis_previsit_conditional;
    v->postvisit_conditional = Analysis_postvisit_conditional;

    v->previsit_whileloop = Analysis_previsit_while_loop;
    v->postvisit_whileloop = Analysis_postvisit_while_loop;

    v->previsit_return = Analysis_previsit_return;
    v->postvisit_return = Analysis_postvisit_return;

    v->previsit_break = Analysis_previsit_break;
    v->postvisit_break = Analysis_postvisit_break;

    v->previsit_continue = Analysis_previsit_continue;
    v->postvisit_continue = Analysis_postvisit_continue;

    v->previsit_binaryop = Analysis_previsit_binop;
    v->invisit_binaryop = Analysis_invisit_binop;
    v->postvisit_binaryop = Analysis_postvisit_binop;

    v->previsit_unaryop = Analysis_previsit_unop;
    v->postvisit_unaryop = Analysis_postvisit_unop;

    v->previsit_location = Analysis_previsit_location;
    v->postvisit_location = Analysis_postvisit_location;

    v->previsit_funccall = Analysis_previsit_funcall;
    v->postvisit_funccall = Analysis_postvisit_funcall;

    v->previsit_literal = Analysis_previsit_literal;
    v->postvisit_literal = Analysis_postvisit_literal;

    // assign a tyoe to all the literals according to chart thing

    /* perform analysis, save error list, clean up, and return errors */
    NodeVisitor_traverse(v, tree);
    ErrorList *errors = ((AnalysisData *)v->data)->errors;
    NodeVisitor_free(v);
    return errors;
}
