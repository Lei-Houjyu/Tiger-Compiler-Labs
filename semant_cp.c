#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "semant.h"

/*Lab4: Your implementation of lab4*/
int loop;
S_symbol loopvar;

struct expty expTy(Tr_exp exp, Ty_ty ty) {
    struct expty e;
    e.exp = exp;
    e.ty = ty;
    return e;
}

struct expty transVar(Tr_level level, Tr_exp exp, S_table venv, S_table tenv, A_var v) {
    switch (v->kind) {
        case A_simpleVar: {
            E_enventry x = S_look(venv, v->u.simple);
            if (x && x->kind == E_varEntry) {
                Tr_exp tr = Tr_simpleVar(x->u.var.access, level);
                return expTy(tr, actual_ty(x->u.var.ty));
            }
            else {
                EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
                return expTy(Tr_emptyExp(), Ty_Int());
            }
        }
        case A_fieldVar: {
            struct expty lvalue = transVar(level, exp, venv, tenv, v->u.field.var);
            if (lvalue.ty->kind == Ty_record)  {
                Ty_fieldList list = lvalue.ty->u.record;
                int offs = 0;
                while (list) {
                    Ty_field item = list->head;
                    if (S_name(item->name) == S_name(v->u.field.sym)) {
                        Tr_exp tr = Tr_fieldVar(lvalue.exp, offs)
                        return expTy(tr, actual_ty(item->ty));
                    }
                    list = list->tail;
                    offs++:
                }
                EM_error(v->pos, "field %s doesn't exist", S_name(v->u.field.sym));
            } else 
                EM_error(v->pos, "not a record type");
            return expTy(Tr_emptyExp(), Ty_Int());
        }
        case A_subscriptVar: {
            struct expty lvalue = transVar(level, exp, venv, tenv, v->u.subscript.var);
            if (lvalue.ty->kind == Ty_array) {
                struct expty sub = transExp(level, exp, venv, tenv, v->u.subscript.exp);
                if (sub.ty->kind ==Ty_int) {
                    Tr_exp tr = Tr_subscriptVar(lvalue, sub);
                    return expTy(tr, actual_ty(lvalue.ty->u.array));
                }
            }
            EM_error(v->pos, "array type required");
            return expTy(Tr_emptyExp(), Ty_Int());
        }
    }
}

struct expty transExp(Tr_level level, Tr_exp exp, S_table venv, S_table tenv, A_exp e) {
    switch (e->kind) {
        case A_varExp: {
            return transVar(level, exp, venv, tenv, e->u.var); 
        }
        case A_nilExp: {
            return expTy(Tr_nilExp(), Ty_Nil());
        }
        case A_intExp: {
            return expTy(Tr_intExp(e->u.intt), Ty_Int());
        }
        case A_stringExp: {
            return expTy(Tr_stringExp(e->u.stringg), Ty_String());
        }
        case A_callExp: {
            E_enventry f = S_look(venv, e->u.call.func);
            if (f && f->kind == E_funEntry) {
                A_expList arg = e->u.call.args;
                Ty_tyList formal = f->u.fun.formals;
                while (arg) {
                    if (!formal) {
                        EM_error(arg->head->pos, "too many params in function %s", S_name(e->u.call.func));
                        break;
                    }
                    struct expty x = transExp(level, exp, venv, tenv, arg->head);
                    if (x.ty->kind != formal->head->kind) 
                        EM_error(arg->head->pos, "para type mismatch");
                    arg = arg->tail;
                    formal = formal->tail;
                }
                if (formal) 
                    EM_error(e->pos, "call exp too little args");
                return expTy(NULL, f->u.fun.result);
            } else {
                EM_error(e->pos, "undefined function %s", S_name(e->u.call.func));
                return expTy(NULL, Ty_Int());
            }
        }
        case A_opExp: {
            A_oper op = e->u.op.oper;
            struct expty left = transExp(level, exp, venv, tenv, e->u.op.left);
            struct expty right = transExp(level, exp, venv, tenv, e->u.op.right);
            switch (op) {
                case A_plusOp:
                case A_minusOp:
                case A_timesOp:
                case A_divideOp: {
                    if (actual_ty(left.ty)->kind != Ty_int) 
                        EM_error(e->u.op.left->pos, "integer required");
                    if (actual_ty(right.ty)->kind != Ty_int)
                        EM_error(e->u.op.right->pos, "integer required");
                    return expTy(Tr_opExp(op, left.exp, right.exp), Ty_Int());
                }
                case A_eqOp:
                case A_neqOp: {
                    if (actual_ty(left.ty)->kind == Ty_int) {
                        if (actual_ty(right.ty)->kind != Ty_int && actual_ty(right.ty)->kind != Ty_nil)
                            EM_error(e->u.op.right->pos, "same type required");
                        return expTy(NULL, Ty_Int());
                    }
                    if (actual_ty(left.ty)->kind == Ty_string) {
                        if (actual_ty(right.ty)->kind != Ty_string && actual_ty(right.ty)->kind != Ty_nil)
                            EM_error(e->u.op.right->pos, "same type required");
                        return expTy(NULL, Ty_String());
                    }
                    if (actual_ty(left.ty)->kind == Ty_array) {
                        if (actual_ty(right.ty)->kind != Ty_array && actual_ty(right.ty)->kind != Ty_nil)
                            EM_error(e->u.op.right->pos, "same type required");
                        return expTy(NULL, Ty_Array(actual_ty(left.ty)->u.array));
                    }
                    if (actual_ty(left.ty)->kind == Ty_record) {
                        if (actual_ty(right.ty)->kind != Ty_record && actual_ty(right.ty)->kind != Ty_nil)
                            EM_error(e->u.op.right->pos, "same type required");
                        return expTy(NULL, Ty_Record(actual_ty(left.ty)->u.record));
                    }
                }
                default: {
                    if (actual_ty(left.ty)->kind == Ty_int) {
                        if (actual_ty(right.ty)->kind != Ty_int && actual_ty(right.ty)->kind != Ty_nil)
                            EM_error(e->u.op.right->pos, "same type required");
                        return expTy(NULL, Ty_Int());
                    }
                    if (actual_ty(left.ty)->kind == Ty_string) {
                        if (actual_ty(right.ty)->kind != Ty_string && actual_ty(right.ty)->kind != Ty_nil)
                            EM_error(e->u.op.right->pos, "same type required");
                        return expTy(NULL, Ty_String());
                    }
                }
            }
        }
        case A_recordExp: {
            Ty_ty x = actual_ty(S_look(tenv, e->u.record.typ));
            if (x && x->kind == Ty_record) {
                Ty_fieldList field = x->u.record;
                A_efieldList efield = e->u.record.fields;
                while (field) {
                    if (!efield || !field) {
                        EM_error(e->pos, "record field amount does not match");
                        break;
                    }
                    if (S_name(field->head->name) != S_name(efield->head->name)) {
                        EM_error(e->pos, "record field name does not match");
                        break;
                    }
                    struct expty exp = transExp(venv, tenv, efield->head->exp);
                    if (exp.ty->kind != field->head->ty->kind) {
                        EM_error(e->pos, "record field type does not match");
                        break;
                    }
                    efield = efield->tail;
                    field = field->tail;
                }
            } else 
                EM_error(e->pos, "undefined type %s", S_name(e->u.record.typ));
            if (!x) return expTy(NULL, Ty_Record(NULL));
            return expTy(NULL, Ty_Record(x->u.record));
        }
        case A_seqExp: {
            A_expList list = e->u.seq;
            struct expty result;
            while (list) {
                result = transExp(level, exp, venv, tenv, list->head);
                list = list->tail;
            }
            return result;
        }
        case A_assignExp: {
            struct expty lvalue = transVar(level, exp, venv, tenv, e->u.assign.var);
            struct expty rvalue = transExp(level, exp, venv, tenv, e->u.assign.exp);
            if (lvalue.ty->kind != rvalue.ty->kind && rvalue.ty->kind !=Ty_nil)
                EM_error(e->pos, "unmatched assign exp");
            if (e->u.assign.var->kind == A_simpleVar &&
                e->u.assign.var->u.simple == loopvar) 
                EM_error(e->pos, "loop variable can't be assigned");
            return expTy(NULL, Ty_Void());
        }
        case A_ifExp: {
            struct expty exp1 = transExp(level, exp, venv, tenv, e->u.iff.test);
            struct expty exp2 = transExp(level, exp, venv, tenv, e->u.iff.then);
            struct expty exp3 = transExp(level, exp, venv, tenv, e->u.iff.elsee);
            if (exp1.ty->kind != Ty_int)
                EM_error(e->pos, "integer expression required");
            if (exp3.ty->kind == Ty_nil) {
                if (exp2.ty->kind != Ty_void)
                    EM_error(e->u.iff.then->pos, "if-then exp's body must produce no value");
                return expTy(NULL, Ty_Void());
            }
            if (exp2.ty->kind != exp3.ty->kind && exp3.ty->kind != Ty_nil)
                EM_error(e->pos, "then exp and else exp type mismatch");
            if (exp2.ty->kind == exp3.ty->kind) 
                return expTy(NULL, exp2.ty);
            return expTy(NULL, Ty_Void());
        }
        case A_whileExp: {
            loop++;
            struct expty exp1 = transExp(level, exp, venv, tenv, e->u.whilee.test);
            struct expty exp2 = transExp(level, exp, venv, tenv, e->u.whilee.body);
            loop--;
            if (exp1.ty->kind != Ty_int)
                EM_error(e->pos, "integer expression required");
            if (exp2.ty->kind != Ty_nil && exp2.ty->kind != Ty_void)
                EM_error(e->pos, "while body must produce no value");
            return expTy(NULL, Ty_Void());
        }
        case A_forExp: {
            loopvar = e->u.forr.var;
            loop++;
            S_beginScope(venv);
            S_enter(venv, e->u.forr.var, E_VarEntry(Ty_Int()));
            struct expty exp1 = transExp(level, exp, venv, tenv, e->u.forr.lo);
            if (exp1.ty->kind != Ty_int)
                EM_error(e->pos, "for exp's range type is not integer");
            struct expty exp2 = transExp(level, exp, venv, tenv, e->u.forr.hi);
            if (exp2.ty->kind != Ty_int)
                EM_error(e->pos, "for exp's range type is not integer");
            struct expty exp3 = transExp(level, exp, venv, tenv, e->u.forr.body);
            loop--;
            if (exp3.ty->kind != Ty_nil && exp3.ty->kind != Ty_void)
                EM_error(e->pos, "for body produces value");
            S_endScope(venv);
            loopvar = NULL;
            return expTy(NULL, Ty_Void());
        }
        case A_breakExp: {
            if (!loop)
                EM_error(e->pos, "break alone");
            return expTy(NULL, Ty_Void());
        }
        case A_letExp: {
            S_beginScope(tenv);
            S_beginScope(venv);

            A_decList list = e->u.let.decs;
            while (list) {
                A_dec dec = list->head;

                switch (dec->kind) {
                    case A_functionDec: {
                        A_fundecList list = dec->u.function;
                        while (list) {
                            A_fundec x = list->head;

                            E_enventry f = S_look(venv, x->name);
                            if (f) {
                                EM_error(x->pos, "two functions have the same name");
                                goto next;
                            }

                            Ty_tyList formals = NULL, current;
                            A_fieldList param = x->params;
                            while (param) {
                                A_field now = param->head;
                                Ty_ty typ = S_look(tenv, now->typ);
                                if (!typ) {
                                    EM_error(now->pos, "fundec params no such type");
                                    typ = Ty_Int();
                                }
                                Ty_tyList one = Ty_TyList(typ, NULL);
                                if (!formals) {
                                    formals = one;
                                    current = formals;
                                } else {
                                    current->tail = one;
                                    current = one;
                                }

                                param = param->tail;
                            }


                            Ty_ty result = Ty_Void();
                            if (x->result) {
                                S_look(tenv, x->result);
                                if (!result) {
                                    EM_error(x->pos, "fundec result no such type");
                                    result = Ty_Int();
                                }
                            }
                            Tr_level l = Tr_newLevel(level, funLabel, makeFormals(fcl->head->params));
                            S_enter(venv, x->name, E_FunEntry(formals, result));

                            next:
                            list = list->tail;
                        }
                        transDec(venv, tenv, dec);
                    } break;
                    case A_typeDec: {
                        A_nametyList nametyList = dec->u.type;
                        while (nametyList) {
                            A_namety now = nametyList->head;

                            Ty_ty x = S_look(tenv, now->name);
                            if (x) 
                                EM_error(dec->pos, "two types have the same name");
                            else
                                S_enter(tenv, now->name, Ty_Name(now->name, NULL));

                            nametyList = nametyList->tail;
                        }
                        transDec(venv, tenv, dec);
                    } break;
                    case A_varDec: 
                        transDec(venv, tenv, dec);
                }
                list = list->tail;
            }

            struct expty exp = transExp(level, exp, venv, tenv, e->u.let.body);

            S_endScope(venv);
            S_endScope(tenv);

            return expTy(NULL, exp.ty);
        }
        case A_arrayExp: {
            Ty_ty ty = actual_ty(S_look(tenv, e->u.array.typ));
            if (!ty || ty->kind != Ty_array)
                EM_error(e->pos, "not array type");
            struct expty exp1 = transExp(level, exp, venv, tenv, e->u.array.size);
            struct expty exp2 = transExp(level, exp, venv, tenv, e->u.array.init);
            if (exp1.ty->kind != Ty_int)
                EM_error(e->u.array.size->pos, "array size is not integer");
            if (ty && exp2.ty->kind != ty->u.array->kind)
                EM_error(e->u.array.init->pos, "type mismatch");
            return expTy(NULL, Ty_Array(ty->u.array));
        }
        default:
            EM_error(e->pos, "undefined expression");
    }
}

void transDec(Tr_level level, Tr_exp exp, S_table venv, S_table tenv, A_dec d) {
    switch (d->kind) {
        case (A_functionDec): {
            A_fundecList function;
            for (function = d->u.function; function; function = function->tail) {
                A_fundec fundec = function->head;
                S_beginScope(venv);

                A_fieldList params;
                for (params = fundec->params; params; params = params->tail) {
                    A_field param = params->head;
                    Ty_ty typ = actual_ty(S_look(tenv, param->typ));
                    if (!typ) {
                        EM_error(fundec->pos, "undefined type");
                        typ = Ty_Int();
                    }
                    S_enter(venv, param->name, E_VarEntry(typ));
                }

                struct expty body = transExp(venv, tenv, fundec->body);

                S_endScope(venv);
                Ty_ty result = Ty_Void();
                if (fundec->result) 
                   result = actual_ty(S_look(tenv, fundec->result));
                if (result) {
                    if (result->kind != body.ty->kind) {
                        if (result->kind == Ty_void)
                            EM_error(fundec->pos, "procedure returns value");
                        else
                            EM_error(fundec->body->pos, "fundec body type conflict");
                    }
                } else
                    EM_error(fundec->pos, "fundec no such return type");
            }
        } break;
        case (A_typeDec): {
            A_nametyList type;
            for (type = d->u.type; type; type = type->tail) {
                A_namety now = type->head;
                Ty_ty namety = S_look(tenv, now->name);
                Ty_ty ty = transTy(tenv, now->ty);
                namety->u.name.ty = ty;
                while (ty && ty->kind == Ty_name) {
                    if (ty->u.name.ty == namety) {
                        EM_error(d->pos, "illegal type cycle");
                        break;
                    }
                    ty = ty->u.name.ty;
                }
            }
        } break;
        case (A_varDec): {
            Ty_ty typ = Ty_Nil();
            if (d->u.var.typ) {
                typ = actual_ty(S_look(tenv, d->u.var.typ));
                if (!typ) {
                    EM_error(d->pos, "vardec error");
                    typ = Ty_Void();
                }
            }
            Tr_access acess = Tr_allocLocal(level, true);
            struct expty exp = transExp(level, exp, venv, tenv, d->u.var.init);
            if (exp.ty->kind != typ->kind && typ->kind != Ty_nil) {
                EM_error(d->pos, "type mismatch");
            }
            if (exp.ty->kind == typ->kind) {
                if (typ->kind == Ty_record && S_name(d->u.var.typ) != S_name(d->u.var.init->u.record.typ))
                    EM_error(d->pos, "type mismatch");               
                if (typ->kind == Ty_array && S_name(d->u.var.typ) != S_name(d->u.var.init->u.array.typ))
                    EM_error(d->pos, "type mismatch");               
            }

            if (exp.ty->kind == Ty_nil && !d->u.var.typ)
                EM_error(d->pos, "init should not be nil without type specified");

            S_enter(venv, d->u.var.var, E_VarEntry(access, exp.ty));
            return Tr_assignExp(Tr_simpleVar(ac, level), final.exp);
        }
    }
}

Ty_ty transTy(S_table tenv, A_ty a) {
    switch (a->kind) {
        case (A_nameTy): {
            Ty_ty ty = S_look(tenv, a->u.name);
            if (!ty)
                EM_error(a->pos, "typedec namety error");
            return ty;
        }
        case (A_recordTy): {
            A_fieldList record;
            Ty_fieldList fieldList = NULL, current;
            for (record = a->u.record; record; record = record->tail) {
                A_field entry = record->head;
                Ty_ty ty = actual_ty(S_look(tenv, entry->typ));
                if (!ty) {
                    EM_error(entry->pos, "undefined type %s", S_name(entry->typ));
                    exit(0);
                }
                Ty_fieldList field = Ty_FieldList(Ty_Field(entry->name, ty), NULL);
                if (!fieldList) {
                    fieldList = field;
                    current = fieldList;
                } else {
                    current->tail = field;
                    current = field;
                }
            }
            return Ty_Record(fieldList);
        }
        case (A_arrayTy): {
            Ty_ty ty = S_look(tenv, a->u.array);
            if (!ty) {
                EM_error(a->pos, "typedec array error");
                ty = Ty_Int();
            }
            return Ty_Array(ty);
        }
    }
}

Ty_ty actual_ty(Ty_ty ty) {
    while (ty && ty->kind == Ty_name) {
        ty = ty->u.name.ty;
        if (!ty) break;
    }
    return ty;
}

F_fragList SEM_transProg(A_exp exp){
    S_table venv = E_base_venv();
    S_table tenv = E_base_tenv();
    Tr_level outermost = Tr_outermost();
    loop = 0;
    S_enter(tenv, S_Symbol("int"), Ty_Int());
    S_enter(tenv, S_Symbol("string"), Ty_String());

    transExp(outermost, NULL, venv, tenv, exp);
    return Tr_getResult();
}
