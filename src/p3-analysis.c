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
 * @brief previsit program node
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_program(NodeVisitor *visitor, ASTNode *node)
{
    if (lookup_symbol(node, "main") == NULL)
    {
        Error_throw_printf("Program does not contain a 'main' function");
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

}

/**
 * @brief previsit vardecl
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_vardecl(NodeVisitor *visitor, ASTNode *node)
{
    if (node->vardecl.type == VOID)
    {
        Error_throw_printf("Void variable '%s' on line %d", node->vardecl.name, node->source_line);
    }

    // check for valid array size in declaration
    if (node->vardecl.is_array)
    {
        if (node->vardecl.array_length <= 0)
        {
            Error_throw_printf("Array length must be greater than 0");
        }
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

}

/**
 * @brief previsit funcdecl
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_funcdecl(NodeVisitor *visitor, ASTNode *node)
{
    SET_INFERRED_TYPE(node->funcdecl.return_type);
    DATA->funcdecl_return_type = node->funcdecl.return_type;
}

/**
 * @brief postvisit funcdecl
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_funcdecl(NodeVisitor *visitor, ASTNode *node)
{

}

/**
 * @brief previsit block
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_block(NodeVisitor *visitor, ASTNode *node)
{

}

/**
 * @brief postvisit block
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_block(NodeVisitor *visitor, ASTNode *node)
{

}

/**
 * @brief previsit assignment
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_assignment(NodeVisitor *visitor, ASTNode *node)
{
    Symbol *sym = lookup_symbol(node, node->assignment.location->location.name);
    DecafType left_type = sym->type;
    DecafType right_type;

    // DecafType literal_type = GET_INFERRED_TYPE(node);
    if (node->assignment.value->type == LOCATION)
    {
        Symbol *sym = lookup_symbol(node, node->assignment.value->location.name);

        // sym will be NULL if there is a VOID type
        if (sym == NULL)
        {
            Error_throw_printf("Expected type %s but got VOID on line %d\n", DecafType_to_string(left_type), node->source_line);
        }
        right_type = sym->type;
    }
    else
    {
        right_type = node->assignment.value->literal.type;
    }

    if (left_type != right_type)
    {
        Error_throw_printf("Expected %s type but type was %s\n", DecafType_to_string(left_type), DecafType_to_string(right_type));
    }
}

/**
 * @brief postisit assignment
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_assignment(NodeVisitor *visitor, ASTNode *node)
{

}

/**
 * @brief previsit conditional
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_conditional(NodeVisitor *visitor, ASTNode *node)
{
    switch (node->conditional.condition->type)
    {
    // binary operators
    case 10:
        break;
    // unary operators
    case 11:
        break;
    // location
    case 12:
        if (lookup_symbol(node, node->conditional.condition->location.name)->type != BOOL)
        {
            Error_throw_printf("Conditional type was %s, expected bool on line %d\n", DecafType_to_string(lookup_symbol(node, node->conditional.condition->location.name)->type), node->source_line);
        }
        break;
    // literal
    case 14:
        if (node->conditional.condition->literal.type != BOOL)
        {
            Error_throw_printf("Conditional type was %s, expected bool on line %d\n", DecafType_to_string(node->conditional.condition->literal.type), node->source_line);
        }
        break;
    default:
        Error_throw_printf("Invalid conditional on line %d", node->source_line);
    }
}

/**
 * @brief postvisit conditional
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_conditional(NodeVisitor *visitor, ASTNode *node)
{

}

/**
 * @brief previsit while loop
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_while_loop(NodeVisitor *visitor, ASTNode *node)
{
    DATA->is_loop = true;
    switch (node->whileloop.condition->type)
    {
    // binary operators
    case 10:
        break;
    // unary operators
    case 11:
        break;
    // location
    case 12:
        if (lookup_symbol(node, node->whileloop.condition->location.name)->type != BOOL)
        {
            Error_throw_printf("Conditional type was %s, expected bool on line %d\n", DecafType_to_string(lookup_symbol(node, node->whileloop.condition->location.name)->type), node->source_line);
        }
        break;
    // literal
    case 14:
        if (node->whileloop.condition->literal.type != BOOL)
        {
            Error_throw_printf("Conditional type was %s, expected bool on line %d\n", DecafType_to_string(node->whileloop.condition->literal.type), node->source_line);
        }
        break;
    default:
        Error_throw_printf("Invalid conditional on line %d", node->source_line);
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
    DATA->is_loop = false;
}

/**
 * @brief previsit return 
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_return(NodeVisitor *visitor, ASTNode *node)
{
    DATA->is_return = true;
    if (node->funcreturn.value->type == LOCATION)
    {
        // do nothing and let previsit_location handle error
    }
    else if (node->funcreturn.value->type == LITERAL)
    {
        if (node->funcreturn.value->literal.type != DATA->funcdecl_return_type)
        {
            Error_throw_printf("Expected %s return type but type was %s\n", DecafType_to_string(DATA->funcdecl_return_type), DecafType_to_string(node->funcreturn.value->literal.type));
        }
    }
    else if (node->funcreturn.value->type == FUNCCALL)
    {
        Error_throw_printf("here");
        // ADD ELSE IF FOR FUNC CALLS
    }
}

/**
 * @brief postvisit return 
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_return(NodeVisitor *visitor, ASTNode *node)
{
    DATA->is_return = false;
}

/**
 * @brief previsit break
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_break(NodeVisitor *visitor, ASTNode *node)
{
    if (!DATA->is_loop)
    {
        Error_throw_printf("Break statement should be inside a while loop.");
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
    if (!DATA->is_loop)
    {
        Error_throw_printf("Continue statement should be inside a while loop.");
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
 * @brief checks if a binary operator's expression evaluates to expected type
 *
 * @param node
 * @param side
 * @param expected_type
 */
void binop_helper1(ASTNode *node ,ASTNode *side, DecafType expected_type)
{
    // check type of expression
    switch (side->type)
    {
    // binary expression
    case 10:
        // let recursion do its thing
        break;
    // literal
    case 14:
        if (side->literal.type != expected_type) 
        {
            Error_throw_printf("Expected %s was %s on line %d\n", DecafType_to_string(expected_type), DecafType_to_string(side->literal.type), node->source_line);
        }
        break;
    // location
    case 12:
        if (lookup_symbol(node, side->location.name)->type != expected_type)
        {
            Error_throw_printf("Expected %s was %s on line %d\n", DecafType_to_string(expected_type), DecafType_to_string(lookup_symbol(node, side->location.name)->type), node->source_line);
        }
        break;
    default:
        Error_throw_printf("Invalid binary expression on line %d", node->source_line);
    }
}

/**
 * @brief returns type of expression for binary operators
 *
 * @param node
 */
DecafType binop_helper2(ASTNode *node)
{
    // check type of expression
    switch (node->type)
    {
    // binary expression
    case 10:
        // let recursion do its thing
        break;
    // literal
    case 14:
        return node->literal.type;
    // location
    case 12:
        return lookup_symbol(node, node->location.name)->type;
    default:
        Error_throw_printf("Invalid binary expression on line %d\n", node->source_line);
    }

    // makes compiler not yell at us
    return INT;
}

/**
 * @brief previsit binop
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_binop(NodeVisitor *visitor, ASTNode *node)
{
    BinaryOpType binop_type = node->binaryop.operator;
    // for >, >=, <, <=, +, -, *, /, %
    if ((binop_type > 3) && (binop_type < 13))
    {
        binop_helper1(node, node->binaryop.left, INT);
        binop_helper1(node, node->binaryop.right, INT);
    // == and !=
    } else if ((binop_type == 2) || (binop_type == 3))
    {
        DecafType left_type = binop_helper2(node->binaryop.left);
        DecafType right_type = binop_helper2(node->binaryop.right);
        if (left_type != right_type)
        {
            Error_throw_printf("Type mismatch cannot use == with %s and %s on line %d\n", DecafType_to_string(left_type), DecafType_to_string(right_type), node->source_line);
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
    
}

/**
 * @brief previsit unary operator
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_unop(NodeVisitor *visitor, ASTNode *node)
{
    
}

/**
 * @brief postvisit unary operator
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_unop(NodeVisitor *visitor, ASTNode *node)
{
    
}

/**
 * @brief previsit location
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_location(NodeVisitor *visitor, ASTNode *node)
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
 * @brief postvisit location
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_location(NodeVisitor *visitor, ASTNode *node)
{

}

/**
 * @brief previsit function call
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_funcall(NodeVisitor *visitor, ASTNode *node)
{

}

/**
 * @brief postvisit function call
 *
 * @param visitor
 * @param node
 */
void Analysis_postvisit_funcall(NodeVisitor *visitor, ASTNode *node)
{

}

/**
 * @brief previsit literal
 *
 * @param visitor
 * @param node
 */
void Analysis_previsit_literal(NodeVisitor *visitor, ASTNode *node)
{
    SET_INFERRED_TYPE(node->literal.type);
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
