/*
 *
 *                This source code is part of
 *                    ******************
 *                    ***   Pteros   ***
 *                    ******************
 *                 molecular modeling library
 *
 * Copyright (c) 2009-2017, Semen Yesylevskyy
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of Artistic License:
 *
 * Please note, that Artistic License is slightly more restrictive
 * then GPL license in terms of distributing the modified versions
 * of this software (they should be approved first).
 * Read http://www.opensource.org/licenses/artistic-license-2.0.php
 * for details. Such license fits scientific software better then
 * GPL because it prevents the distribution of bugged derivatives.
 *
*/

#ifndef SELECTION_GRAMMAR_H
#define SELECTION_GRAMMAR_H

#include "selection_parser.h"
#include <functional>
#include <map>
#include "pteros/core/pteros_error.h"

//===========================================================
using namespace std;
using namespace std::placeholders;
using namespace pteros;

#ifdef _DEBUG_PARSER
#define DEBUG(code) code
#else
#define DEBUG(code)
#endif


#ifdef _DEBUG_PARSER
/*===================================================*/
/*   Rule macros for debug mode - with rule names    */
/*===================================================*/

// Rule
#define RULE(_name) \
bool _name(bool add_to_tree = true, bool do_memo = true){ \
    return rule_proxy(__LINE__, /*rule_id*/ \
                      add_to_tree, do_memo, \
                      false,  /* do_reduce */ \
                      "",     /* literal */ \
                      std::bind(&Grammar::_name##_body,this,_1,_2,_3), \
                      #_name);   /* rule_name */ \
}\
void _name##_body(AstNode_ptr& _this_rule_, bool& _ok_, const std::string::iterator& _old_) \

// Reduction rule
#define RULE_REDUCE(_name) \
bool _name(bool add_to_tree = true, bool do_memo = true){ \
    return rule_proxy(__LINE__, /*rule_id*/ \
                      add_to_tree, do_memo, \
                      true,  /* do_reduce */ \
                      "",     /* literal */ \
                      std::bind(&Grammar::_name##_body,this,_1,_2,_3), \
                      #_name);   /* rule_name */ \
}\
void _name##_body(AstNode_ptr& _this_rule_, bool& _ok_, const std::string::iterator& _old_) \

// Literal rule
#define LITERAL_RULE(_name, _lit) \
bool _name(bool add_to_tree = true, bool do_memo = false){ \
    return rule_proxy(__LINE__, /*rule_id*/ \
                      add_to_tree, do_memo, \
                      false,  /* do_reduce */ \
                      _lit,     /* literal */ \
                      std::bind(&Grammar::_name##_body,this,_1,_2,_3), \
                      #_name);   /* rule_name */ \
}\
void _name##_body(AstNode_ptr& _this_rule_, bool& _ok_, const std::string::iterator& _old_) \

#else

/*========================================================*/
/*   Rule macros for release mode - without rule names    */
/*========================================================*/

// Rule
#define RULE(_name) \
bool _name(bool add_to_tree = true, bool do_memo = true){ \
    return rule_proxy(__LINE__, /*rule_id*/ \
                      add_to_tree, do_memo, \
                      false,  /* do_reduce */ \
                      "",     /* literal */ \
                      std::bind(&Grammar::_name##_body,this,_1,_2,_3) ); \
}\
void _name##_body(AstNode_ptr& _this_rule_, bool& _ok_, const std::string::iterator& _old_) \

// Reduction rule
#define RULE_REDUCE(_name) \
bool _name(bool add_to_tree = true, bool do_memo = true){ \
    return rule_proxy(__LINE__, /*rule_id*/ \
                      add_to_tree, do_memo, \
                      true,  /* do_reduce */ \
                      "",     /* literal */ \
                      std::bind(&Grammar::_name##_body,this,_1,_2,_3) ); \
}\
void _name##_body(AstNode_ptr& _this_rule_, bool& _ok_, const std::string::iterator& _old_) \

// Literal rule
#define LITERAL_RULE(_name, _lit) \
bool _name(bool add_to_tree = true, bool do_memo = false){ \
    return rule_proxy(__LINE__, /*rule_id*/ \
                      add_to_tree, do_memo, \
                      false,  /* do_reduce */ \
                      _lit,     /* literal */ \
                      std::bind(&Grammar::_name##_body,this,_1,_2,_3) ); \
}\
void _name##_body(AstNode_ptr& _this_rule_, bool& _ok_, const std::string::iterator& _old_) \

#endif



#define SUBRULE(n) (_this_rule_->child_node(n))

#define NUM_SUBRULES() (_this_rule_->children.size())

/*===================================*/
/*           PREDICATES              */
/*===================================*/

// Predicate, which combines sequence of rules. If sequence fails it rewinds _pos_ to beginning
#define Comb(rule) \
    ( \
    [this,&_this_rule_]()->bool{ \
        std::string::iterator old=_pos_; \
        int old_sz = NUM_SUBRULES(); \
        bool ok = (rule);\
        if(!ok) { \
            _pos_ = old; \
            _this_rule_->children.resize(old_sz); \
        } \
        return ok; \
    }() \
    )

// Make rule optional
#define Opt(rule) (Comb(rule)||true)

// Predicate, which checks if rule matches, but doesn't advance iterator
#define Check(rule) \
    ( \
    [this,&_this_rule_]()->bool{ \
        std::string::iterator old=_pos_; \
        int old_sz = NUM_SUBRULES(); \
        bool ok = (rule);\
        _pos_ = old; \
        _this_rule_->children.resize(old_sz); \
        return ok; \
    }() \
    )

#define ZeroOrMore(rule) \
    ( \
    [this,&_this_rule_]()->bool{ \
        bool ok = true; \
        while(ok){ ok = Comb(rule); } \
        return true; \
    }() \
    )

#define OneOrMore(rule) \
    ( \
    [this,&_this_rule_]()->bool{ \
        bool ok = Comb(rule); \
        if(!ok) return false; \
        while(ok){ ok = Comb(rule); } \
        return true; \
    }() \
    )

/*===================================*/
/*           Memo table              */
/*===================================*/

struct Memo_data {
    Memo_data(){}
    Memo_data(bool _ok, const AstNode_ptr& _tree, const string::iterator& _pos):
        ok(_ok), tree(_tree), pos(_pos) {}

    bool ok; // Rule evaluation result
    AstNode_ptr tree;  // Subtree
    string::iterator pos;  // iterator after rule completion
};



/*===================================*/
/*           THE GRAMMAR             */
/*===================================*/

class Grammar {
private:
    std::string::iterator _pos_,beg,end,last_success;
    AstNode_ptr current_parent;
    // Memo table
    map<int, map<int,Memo_data> > memo;

#ifdef _DEBUG_PARSER
    int level; // For pretty printing
    int num_stored, num_restored;
    int max_level;
    int num_tried;
    int num_failed;
#endif

public:
    Grammar(std::string& s){
        _pos_ = beg = last_success = s.begin();
        end = s.end();

#ifdef _DEBUG_PARSER
        level = 0;
        num_stored = 0;
        num_restored = 0;
        max_level = 0;
        num_tried = 0;
        num_failed = 0;
#endif
    }

#ifdef _DEBUG_PARSER
    bool rule_proxy(const int rule_id, bool add_to_tree, bool do_memo, bool do_reduce, const string& literal,
                    std::function<void(AstNode_ptr&,bool&,const std::string::iterator&)> user_defined_body,
                    std::string rule_name=""){
#else
    bool rule_proxy(const int rule_id, bool add_to_tree, bool do_memo, bool do_reduce, const string& literal,
                    std::function<void(AstNode_ptr&,bool&,const std::string::iterator&)> user_defined_body){
#endif
        if(_pos_==end) return false;

        bool _ok_ = false;

        AstNode_ptr _this_rule_(new AstNode);

        AstNode_ptr saved_parent = current_parent;
        current_parent = _this_rule_;

        DEBUG(_this_rule_->name = rule_name;)

        int n = std::distance(beg,_pos_);

        DEBUG(for(int i=0;i<level;++i) cout <<"  ";)
        DEBUG(cout << "Trying " << _this_rule_->name << " id: " << rule_id << " at: " << n << endl;)
        DEBUG(num_tried++;)

        /* check memotable */
        bool restored = false;
        if(literal!="") do_memo = (literal.size()>3) ? true : false; /*cache only 'large' literals*/
        if(do_memo && memo[rule_id].count(n)==1){
            DEBUG(for(int i=0;i<level;++i) cout <<"  ";)
            DEBUG(cout << "-- from memo: " << _this_rule_->name << " at: " << n << endl;)
            Memo_data& m = memo[rule_id][n];
            _this_rule_ = m.tree;
            _pos_ = m.pos;
            _ok_ = m.ok;
            restored = true;
            DEBUG(num_restored++;)
        }
        DEBUG(level++;)
        DEBUG(if(max_level<level) max_level = level;)

        /* variables for rule body */
        std::string::iterator _old_ = _pos_;

        if(!restored){

            // For literals
            if(literal!=""){
                for(auto ch: literal){
                    if(*_pos_==ch){
                        _pos_++;
                    } else {
                        break;
                    }
                }
                if(_pos_-_old_== literal.size()) _ok_ = true;
                if(!_ok_) _pos_ = _old_;
            }

            // Call user-defined rule body
            user_defined_body(_this_rule_,_ok_,_old_);
        }

        if(_ok_){
            if(_pos_>last_success) last_success = _pos_;
            DEBUG(for(int i=0;i<level-1;++i) cout <<"  ";)
            DEBUG(cout<<"Matched "<< _this_rule_->name << " at: " << n << endl;)
            if(add_to_tree) {
                if(do_reduce && _this_rule_->children.size()==1){
                    saved_parent->children.push_back(_this_rule_->children[0]);
                    DEBUG(for(int i=0;i<level-1;++i) cout <<"  ";)
                    DEBUG(cout << "-- simplifying node "<<_this_rule_->name << " to parent " << saved_parent->name << endl;)
                } else {
                    saved_parent->children.push_back(_this_rule_);
                    DEBUG(for(int i=0;i<level-1;++i) cout <<"  ";)
                    DEBUG(cout << "-- adding node "<<_this_rule_->name << " to parent " << saved_parent->name << endl;)
                }
            }
        } else {
            DEBUG(for(int i=0;i<level-1;++i) cout <<"  ";)
            DEBUG(cout<<"Failed "<< _this_rule_->name << " at: " << n << endl;)
            _pos_ = _old_;
            _this_rule_->children.clear();
            DEBUG(num_failed++;)
        }
        current_parent = saved_parent;
        /* Add to memotable */
        if(do_memo && !restored) {
           DEBUG(for(int i=0;i<level-1;++i) cout <<"  ";)
           DEBUG(cout << "-- to memo: " << _this_rule_->name << " at: " << n << endl;)
           memo[rule_id][n] = Memo_data(_ok_,_this_rule_,_pos_);
           DEBUG(num_stored++;)
        }
        DEBUG(level--;)
        DEBUG(cout << endl;)
        return _ok_;
    }

    /*===================================*/
    /*           TERMINALS               */
    /*===================================*/


    RULE(SPACE){
        while(isspace(*_pos_)) _pos_++;
        if(_old_!=_pos_) _ok_ = true;
    }

// Optional space
#define SP_() (SPACE(false,false)||true)
// Mandatory space
#define SP() (SPACE(false,false))

    RULE(UINT){
        char* e;
        int val = strtoul(&*_old_, &e, 0);
        _pos_ += e-&*_old_;
        if(_old_!=_pos_) _ok_ = true;
        SP_(); // Consume any trailing space if present

        if(_ok_){
            _this_rule_->code = TOK_UINT;
            _this_rule_->children.push_back(val);
        } // _ok_
    }

    RULE(INT){
        char* e;
        int val = strtol(&*_old_, &e, 0);
        _pos_ += e-&*_old_;
        if(_old_!=_pos_) _ok_ = true;
        SP_(); // Consume any trailing space if present

        if(_ok_){
            _this_rule_->code = TOK_INT;
            _this_rule_->children.push_back(val);
        } // _ok_
    }

    RULE(FLOAT){
        char* e;
        float val = strtod(&*_old_, &e);
        _pos_ += e-&*_old_;
        if(_old_!=_pos_) _ok_ = true;
        SP_(); // Consume any training space if present

        if(_ok_){
            _this_rule_->code = TOK_FLOAT;
            _this_rule_->children.push_back(val);
        } // _ok_
    }


    /*===================================*/
    /*           LITERALS                */
    /*===================================*/

    LITERAL_RULE(PLUS,"+"){
        _this_rule_->code = TOK_PLUS;
    }

    LITERAL_RULE(MINUS,"-"){
        _this_rule_->code = TOK_MINUS;
    }

    LITERAL_RULE(STAR,"*"){
        _this_rule_->code = TOK_MULT;
    }

    LITERAL_RULE(SLASH,"/"){
        _this_rule_->code = TOK_DIV;
    }

    LITERAL_RULE(CAP,"^"){
        _this_rule_->code = TOK_POWER;
    }

    LITERAL_RULE(DOUBLE_STAR,"**"){
        _this_rule_->code = TOK_POWER;
    }

    LITERAL_RULE(LPAREN,"("){}

    LITERAL_RULE(RPAREN,")"){}

    LITERAL_RULE(X,"x"){
        _this_rule_->code = TOK_X;        
    }

    LITERAL_RULE(Y,"y"){
        _this_rule_->code = TOK_Y;
    }

    LITERAL_RULE(Z,"z"){
        _this_rule_->code = TOK_Z;
    }

    LITERAL_RULE(BETA,"beta"){
        _this_rule_->code = TOK_BETA;
    }

    LITERAL_RULE(OCCUPANCY,"occupancy"){
        _this_rule_->code = TOK_OCC;
    }

    LITERAL_RULE(DIST1,"distance"){}

    LITERAL_RULE(DIST2,"dist"){}

    LITERAL_RULE(POINT,"point"){}

    LITERAL_RULE(VECTOR,"vector"){}

    LITERAL_RULE(PLANE,"plane"){}

    LITERAL_RULE(OR,"or"){
        _this_rule_->code = TOK_OR;
    }

    LITERAL_RULE(AND,"and"){
        _this_rule_->code = TOK_AND;
    }

    LITERAL_RULE(ALL,"all"){
        _this_rule_->code = TOK_ALL;
    }

    LITERAL_RULE(EQ,"=="){
        _this_rule_->code = TOK_EQ;
    }

    LITERAL_RULE(NEQ,"!="){
        _this_rule_->code = TOK_NEQ;
    }

    LITERAL_RULE(GEQ,">="){
        _this_rule_->code = TOK_GEQ;
    }

    LITERAL_RULE(LEQ,"<="){
        _this_rule_->code = TOK_LEQ;
    }

    LITERAL_RULE(GT,">"){
        _this_rule_->code = TOK_GT;
    }

    LITERAL_RULE(LT,"<"){
        _this_rule_->code = TOK_LT;     
    }

    LITERAL_RULE(NOT,"not"){}

    LITERAL_RULE(WITHIN_,"within"){}

    LITERAL_RULE(OF,"of"){}

    LITERAL_RULE(SELF_ON,"self"){
        _this_rule_->code = TOK_SELF;
        _this_rule_->children.push_back(1);
    }

    LITERAL_RULE(SELF_OFF,"noself"){
        _this_rule_->code = TOK_SELF;
        _this_rule_->children.push_back(0);
    }

    LITERAL_RULE(PBC_ON1,"pbc"){
        _this_rule_->code = TOK_UINT;
        _this_rule_->children.push_back(1);
    }

    LITERAL_RULE(PBC_ON2,"periodic"){
        _this_rule_->code = TOK_UINT;
        _this_rule_->children.push_back(1);
    }

    LITERAL_RULE(PBC_OFF1,"nopbc"){
        _this_rule_->code = TOK_UINT;
        _this_rule_->children.push_back(0);
    }

    LITERAL_RULE(PBC_OFF2,"noperiodic"){
        _this_rule_->code = TOK_UINT;
        _this_rule_->children.push_back(0);
    }

    LITERAL_RULE(BY,"by"){}

    LITERAL_RULE(TO,"to"){}

    LITERAL_RULE(RESIDUE,"residue"){}

    LITERAL_RULE(NAME,"name"){
        _this_rule_->code = TOK_NAME;
    }

    LITERAL_RULE(RESNAME,"resname"){
        _this_rule_->code = TOK_RESNAME;
    }

    LITERAL_RULE(TAG,"tag"){
        _this_rule_->code = TOK_TAG;
    }

    LITERAL_RULE(CHAIN,"chain"){
        _this_rule_->code = TOK_CHAIN;
    }

    LITERAL_RULE(RESID,"resid"){
        _this_rule_->code = TOK_RESID;
    }

    LITERAL_RULE(RESINDEX,"resindex"){
        _this_rule_->code = TOK_RESINDEX;
    }

    LITERAL_RULE(INDEX,"index"){
        _this_rule_->code = TOK_INDEX;
    }

    /*===================================*/
    /*           NON-TERMINALS           */
    /*===================================*/


    RULE_REDUCE(NUM_EXPR){

        _ok_ = NUM_TERM() && ZeroOrMore( (PLUS() || MINUS()) && SP_() && NUM_TERM() );

        if(_ok_){
            if(NUM_SUBRULES()>1){ // Some operations present
                AstNode_ptr tmp = SUBRULE(0);
                for(int i=1; i<NUM_SUBRULES()-1; i+=2){
                    tmp.swap( SUBRULE(i) ); // i is now left operand, tmp is operation
                    tmp->children.push_back(SUBRULE(i)); // lelf operand
                    tmp->children.push_back(SUBRULE(i+1)); // right operand
                }
                _this_rule_ = tmp;
            }
        } // _ok_
    }

    RULE_REDUCE(NUM_TERM){

        _ok_ = NUM_POWER() && ZeroOrMore( (STAR() || SLASH()) && SP_() && NUM_POWER() );

        if(_ok_){
            if(NUM_SUBRULES()>1){ // Some operations present
                AstNode_ptr tmp = SUBRULE(0);
                for(int i=1; i<NUM_SUBRULES()-1; i+=2){                    
                    tmp.swap( SUBRULE(i) ); // i is now left operand, tmp is operation
                    tmp->children.push_back(SUBRULE(i)); // lelf operand
                    tmp->children.push_back(SUBRULE(i+1)); // right operand
                }
                _this_rule_ = tmp;
            }
        } // _ok_
    }


    RULE_REDUCE(NUM_POWER){

        _ok_ = NUM_FACTOR() && Opt( (CAP(false) || DOUBLE_STAR(false)) && SP_()&& NUM_FACTOR() );

        if(_ok_){
            if(NUM_SUBRULES()>1){
                _this_rule_->code = TOK_POWER; // Set operation, no other actions needed
            }
        } // _ok_
    }

    RULE_REDUCE(NUM_FACTOR){
        _ok_ = Comb( LPAREN(false) && SP_() && NUM_EXPR() && RPAREN(false) && SP_() )
                || FLOAT()                
                || Comb( X() && SP_() )
                || Comb( Y() && SP_() )
                || Comb( Z() && SP_() )
                || Comb( BETA() && SP_() )
                || Comb( OCCUPANCY() && SP_() )
                || Comb( RESINDEX() && SP_() )
                || Comb( INDEX() && SP_() )
                || Comb( RESID() && SP_() )
                || UNARY_MINUS()
                || DIST_POINT()
                || DIST_VECTOR()
                || DIST_PLANE()
                ;        
    }

    RULE(UNARY_MINUS){
        _ok_ = MINUS(false) && NUM_FACTOR();

        if(_ok_){
            _this_rule_->code = TOK_UNARY_MINUS;
        } // _ok_
    }

    RULE(DIST_POINT){
        _ok_ = (DIST1(false) || DIST2(false)) && SP()
                && POINT(false) && SP()
                && Opt(PBC())
                && FLOAT() && FLOAT() && FLOAT();

        if(_ok_){
            _this_rule_->code = TOK_POINT;
            if(NUM_SUBRULES()==3){ // No pbc given
                _this_rule_->children.push_back(0); // add default pbc at the end
            } else {
                // Put pbc to the end
                std::rotate(_this_rule_->children.begin(),
                            _this_rule_->children.begin()+1,
                            _this_rule_->children.end());
                // Reduce pbc node to number 0 or 1
                _this_rule_->children[3] = _this_rule_->child_as_int(3);                
            }
            // Reduce 3 first nodes to floats
            for(int i=0;i<3;++i) _this_rule_->children[i] = _this_rule_->child_as_float(i);
        } // _ok_
    }

    RULE(DIST_VECTOR){
        _ok_ = (DIST1(false) || DIST2(false)) && SP()
                && VECTOR(false) && SP()
                && Opt(PBC())
                && FLOAT() && FLOAT() && FLOAT() && FLOAT() && FLOAT() && FLOAT();

        if(_ok_){
            _this_rule_->code = TOK_VECTOR;
            if(NUM_SUBRULES()==6){ // No pbc given
                _this_rule_->children.push_back(0); // add default pbc at the end
            } else {
                // Put pbc to the end
                std::rotate(_this_rule_->children.begin(),
                            _this_rule_->children.begin()+1,
                            _this_rule_->children.end());
                // Reduce pbc node to number 0 or 1
                _this_rule_->children[6] = _this_rule_->child_as_int(6);
            }
            // Reduce 6 first nodes to floats
            for(int i=0;i<6;++i) _this_rule_->children[i] = _this_rule_->child_as_float(i);
        } // _ok_
    }

    RULE(DIST_PLANE){
        _ok_ = (DIST1(false) || DIST2(false)) && SP()
                && PLANE(false) && SP()
                && Opt(PBC())
                && FLOAT() && FLOAT() && FLOAT() && FLOAT() && FLOAT() && FLOAT();

        if(_ok_){
            _this_rule_->code = TOK_PLANE;
            if(NUM_SUBRULES()==6){ // No pbc given
                _this_rule_->children.push_back(0); // add default pbc at the end
            } else {
                // Put pbc to the end
                std::rotate(_this_rule_->children.begin(),
                            _this_rule_->children.begin()+1,
                            _this_rule_->children.end());
                // Reduce pbc node to number 0 or 1
                _this_rule_->children[6] = _this_rule_->child_as_int(6);
            }
            // Reduce 6 first nodes to floats
            for(int i=0;i<6;++i) _this_rule_->children[i] = _this_rule_->child_as_float(i);
        } // _ok_
    }

    RULE_REDUCE(LOGICAL_EXPR){

        _ok_ = LOGICAL_OPERAND() && ZeroOrMore( (OR() || AND()) && SP_() && LOGICAL_OPERAND() );

        if(_ok_){
            if(NUM_SUBRULES()>1){ // Some operations present
                AstNode_ptr tmp = SUBRULE(0);
                for(int i=1; i<NUM_SUBRULES()-1; i+=2){
                    tmp.swap( SUBRULE(i) ); // i is now left operand, tmp is operation
                    tmp->children.push_back(SUBRULE(i)); // lelf operand
                    tmp->children.push_back(SUBRULE(i+1)); // right operand
                }
                _this_rule_ = tmp;
            }
        } // _ok_
    }

    RULE_REDUCE(LOGICAL_OPERAND){
        _ok_ = Comb( LPAREN(false) && SP_() && LOGICAL_EXPR() && RPAREN(false) && SP_() )
                ||
               Comb( !Check(NUM_EXPR(false) && !COMPARISON_OPERATOR(false)) && NUM_COMPARISON() )
                ||
               Comb( ALL() && SP_() )
                ||
               LOGICAL_NOT()
                ||
               WITHIN()
                ||
               BY_RESIDUE()
                ||
               KEYWORD_LIST_STR()
                ||
               KEYWORD_INT_STR()
                ||
               RESID_RULE()
                ;        
    }

    RULE_REDUCE(COMPARISON_OPERATOR){
        _ok_ = (EQ() || NEQ() || LEQ() || GEQ() || LT() || GT()) && SP_();        
    }


    RULE_REDUCE(NUM_COMPARISON){

        _ok_ = NUM_EXPR();

        Comb( COMPARISON_OPERATOR() && NUM_EXPR() && COMPARISON_OPERATOR() && NUM_EXPR()) //chained
        ||
        Comb( COMPARISON_OPERATOR() && NUM_EXPR() ); // normal


        if(_ok_){
            // single NUM_EXPR will pass through and will be reduced. No action needed.
            if(NUM_SUBRULES()==3){ // normal comparison
                SUBRULE(1)->children.push_back(SUBRULE(0));
                SUBRULE(1)->children.push_back(SUBRULE(2));
                _this_rule_ = SUBRULE(1);
            } else { // chained comparison
                AstNode_ptr op1 = SUBRULE(1);
                op1->children.push_back(SUBRULE(0));
                op1->children.push_back(SUBRULE(2));
                AstNode_ptr op2 = SUBRULE(3);
                op2->children.push_back(SUBRULE(2));
                op2->children.push_back(SUBRULE(4));                
                _this_rule_->code = TOK_AND;
                _this_rule_->children.clear();
                _this_rule_->children.push_back(op1);
                _this_rule_->children.push_back(op2);
            }
        } // _ok_;
    }

    RULE(LOGICAL_NOT){
        _ok_ = NOT(false) && SP_() && LOGICAL_OPERAND();

        if(_ok_){
            _this_rule_->code = TOK_NOT;
        } // _ok_
    }

    RULE(WITHIN){
        _ok_ = WITHIN_(false) && SP_() && FLOAT() && SP_()
                && Opt(Comb(PBC() && SELF()) || Comb(SELF() && PBC()) || PBC() || SELF())
                && OF(false)
                && (SP()||Check(LPAREN(false))) && LOGICAL_OPERAND();

        if(_ok_){
            _this_rule_->code = TOK_WITHIN;
            if(NUM_SUBRULES()==2){ // no pbc, no self given only (d,operand)
                _this_rule_->children.push_back(0); // add nopbc at the end
                _this_rule_->children.push_back(1); // add self at the end
            } else if(NUM_SUBRULES()==3) { // (d,pbc,operand) or (d,self,operand)
                if(SUBRULE(1)->code == TOK_SELF){
                    // We have (d,self,op)
                    SUBRULE(1).swap( SUBRULE(2) ); // now: (d,op,self)
                    // insert nopbc before self
                    _this_rule_->children.insert(_this_rule_->children.begin()+2, 0);
                    // And reduce self to int
                    _this_rule_->children[3] = _this_rule_->child_as_int(3);
                } else {
                    // We have (d,pbc,op)
                    SUBRULE(1).swap( SUBRULE(2) ); // now: (d,op,pbc)
                    // Reduce pbc node to number 0 or 1
                    _this_rule_->children[2] = _this_rule_->child_as_int(2);
                    // Add default self
                    _this_rule_->children.push_back(1);
                }

            } else {
                // We have (d,self,pbc,op) OR (d,pbc,self,op)
                if(SUBRULE(1)->code == TOK_SELF){
                    // We have (d,self,pbc,op)
                    SUBRULE(1).swap( SUBRULE(3) ); // now: (d,op,pbc,self)
                } else {
                    // We have (d,pbc,self,op)
                    SUBRULE(1).swap( SUBRULE(3) ); // now: (d,op,self,pbc)
                    SUBRULE(2).swap( SUBRULE(3) ); // now: (d,op,pbc,pbc)
                }
                // Reduce both pbc and self
                _this_rule_->children[2] = _this_rule_->child_as_int(2);
                _this_rule_->children[3] = _this_rule_->child_as_int(3);
            }

            // Reduce dist node to float
            _this_rule_->children[0] = _this_rule_->child_as_float(0);
        } // _ok_
    }

    RULE_REDUCE(PBC){
        _ok_ = (PBC_ON1() || PBC_ON2() || PBC_OFF1() || PBC_OFF2()) && SP_();
    }

    RULE_REDUCE(SELF){
        _ok_ = (SELF_ON() || SELF_OFF()) && SP_();
    }

    RULE(BY_RESIDUE){
        _ok_ = BY(false) && SP() && RESIDUE(false) && SP_() && LOGICAL_OPERAND();

        if(_ok_){
            _this_rule_->code = TOK_BY;
        } // _ok_
    }

    RULE(KEYWORD_LIST_STR){
        _ok_ = STR_KEYWORD() && SP() && OneOrMore( STR()||REGEX() );

        if(_ok_){
            for(int i=1; i<NUM_SUBRULES(); ++i){
                SUBRULE(0)->children.push_back(SUBRULE(i));
            }
            _this_rule_ = SUBRULE(0);
        } // _ok_
    }

    RULE(KEYWORD_INT_STR){
        _ok_ = INT_KEYWORD() && SP() && OneOrMore( RANGE()||UINT() && SP_() );

        if(_ok_){
            for(int i=1; i<NUM_SUBRULES(); ++i){
                SUBRULE(0)->children.push_back(SUBRULE(i));
            }
            _this_rule_ = SUBRULE(0);
        } // _ok_
    }

    // resid could be negative in some funny structures, so INT, not UINT
    RULE(RESID_RULE){
        _ok_ = RESID() && SP() && OneOrMore( RANGE_SIGNED()||INT() && SP_() );

        if(_ok_){
            for(int i=1; i<NUM_SUBRULES(); ++i){
                SUBRULE(0)->children.push_back(SUBRULE(i));
            }
            _this_rule_ = SUBRULE(0);
        } // _ok_
    }

    RULE_REDUCE(STR_KEYWORD){
        _ok_ = NAME() || RESNAME() || TAG() || CHAIN();
    }

    RULE_REDUCE(INT_KEYWORD){
        _ok_ = RESINDEX() || INDEX();
    }

    RULE(STR){
        _ok_ = !Check( OR(false) || AND(false) );
        if(_ok_){
            while(isalnum(*_pos_) && _pos_!=end){
                _pos_++;
            }
        }

        string s;

        if(_pos_!=_old_){
            if(_ok_) s = string(_old_,_pos_);
            _ok_ = SP() || Check(RPAREN(false)) || Check(MINUS(false)) || (_pos_==end);
        } else {
            _ok_ = false;
        }

        if(_ok_){
            _this_rule_->code = TOK_STR;
            _this_rule_->children.push_back(s);
        } // _ok_
    }

    RULE(REGEX){
        _ok_ = (*_pos_=='\'');
        if(_ok_){
            _pos_++;
            while(*_pos_!='\'' && _pos_!=end) _pos_++;
            if(_pos_!=_old_){
                _ok_ = (*_pos_=='\'');
                if(_ok_) _pos_++;
            }
        } else {
            _ok_ = (*_pos_=='"');
            if(_ok_){
                _pos_++;
                while(*_pos_!='"' && _pos_!=end) _pos_++;
                if(_pos_!=_old_){
                    _ok_ = (*_pos_=='"');
                    if(_ok_) _pos_++;
                }
            }
        }

        string::iterator b = _old_+1;
        string::iterator e = _pos_-1;        

        SP_(); // Consume any trailing space if present

        if(_ok_){
            _this_rule_->code = TOK_REGEX;
            _this_rule_->children.push_back(string(b,e));
        } // _ok_
    }

    RULE(RANGE){
        _ok_ = UINT() && SP_() && (TO(false)||MINUS(false)) && SP_() && UINT();
        if(_ok_){
            _this_rule_->code = TOK_TO;
        } // _ok_
    }

    RULE(RANGE_SIGNED){
        _ok_ = INT() && SP_() && (TO(false)||MINUS(false)) && SP_() && INT();
        if(_ok_){
            _this_rule_->code = TOK_TO;
        } // _ok_
    }


    RULE_REDUCE(START){
        _ok_ = SP_() && LOGICAL_EXPR();        
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    AstNode_ptr run(){
        current_parent.reset(new AstNode); // Hierarchy root

        START(true,false);

#ifdef _DEBUG_PARSER
        cout << "Statistics:" << endl;
        cout << "Number of used rules: " << memo.size() << endl;
        cout << "Recursion depth: " << max_level << endl;
        cout << "Size of memotable: " << memo.size() << endl;
        cout << "Rules stored to memotable: " << num_stored << endl;
        cout << "Rules restored from memotable: " << num_restored << endl;
        cout << "Rules tried: " << num_tried << endl;
        cout << "Rules succeded: " << num_tried-num_failed << endl;
        cout << "Rules failed: " << num_failed << endl;
#endif

        if(current_parent->children.size()>0 && _pos_== end)
            return current_parent->child_node(0);
        else {            
            int n = std::distance(beg,last_success);
            stringstream ss;
            ss << "Syntax error in selection! Somewhere after position " << n << ":" << endl;
            ss << string(beg,end) << endl;
            for(int i=0;i<n-1;i++) ss<< "-";
            ss << "^" << endl;

            throw Pteros_error(ss.str());
        }        
    }

};


//===========================================================

#endif /* SELECTION_GRAMMAR_H */
