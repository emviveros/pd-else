
//#include <string.h>
#include "m_pd.h"
#include <stdlib.h>

#define INISIZE 32

typedef struct _limit{
    t_object     x_obj;
    int          x_open;
    t_float      x_delta;
    t_symbol    *x_selector;
    t_float      x_float;
    t_symbol    *x_symbol;
    int          x_size;    // allocated size
    int          x_natoms;  // used size
    t_atom      *x_message;
    int          x_entered;
    t_clock     *x_clock;
}t_limit;

static t_class *limit_class;

static void limit_dooutput(t_limit *x, t_symbol *s, int ac, t_atom *av){
    x->x_open = 0;     /* so there will be no reentrant calls of dooutput */
    x->x_entered = 1;  /* this prevents a message from being overridden */
    clock_unset(x->x_clock);
    if(s == &s_bang)
        outlet_bang(((t_object *)x)->ob_outlet);
    else if(s == &s_float)
        outlet_float(((t_object *)x)->ob_outlet, x->x_float);
    else if(s == &s_symbol && x->x_symbol){
	/* if x_symbol is null, then symbol &s_ is passed
	   by outlet_anything() -> typedmess() */
        outlet_symbol(((t_object *)x)->ob_outlet, x->x_symbol);
        x->x_symbol = 0;
    }
    else if(s == &s_list)
        outlet_list(((t_object *)x)->ob_outlet, &s_list, ac, av);
    else if(s)
        outlet_anything(((t_object *)x)->ob_outlet, s, ac, av);
    x->x_selector = 0;
    x->x_natoms = 0;
    if(x->x_delta > 0)
        clock_delay(x->x_clock, x->x_delta);
    else
        x->x_open = 1;
    x->x_entered = 0;
}

static void limit_tick(t_limit *x){
    if(x->x_selector)
        limit_dooutput(x, x->x_selector, x->x_natoms, x->x_message);
    else
        x->x_open = 1;
}

static void limit_anything(t_limit *x, t_symbol *s, int ac, t_atom *av){
    if(x->x_open)
        limit_dooutput(x, s, ac, av);
    else if(s && s != &s_ && !x->x_entered){
        if(ac > x->x_size){ // no MAXSIZE
            x->x_message = realloc(x->x_message, sizeof(t_atom)*ac);
            x->x_size = ac;
        }
        x->x_selector = s;
        x->x_natoms = ac;
        if(ac)
            memcpy(x->x_message, av, ac * sizeof(*x->x_message));
    }
}

static void limit_bang(t_limit *x){
    x->x_selector = &s_bang;
    limit_anything(x, x->x_selector, 0, 0);
}

static void limit_float(t_limit *x, t_float f){
    x->x_selector = &s_float;
    x->x_float = f;
    limit_anything(x, x->x_selector, 0, 0);
}

static void limit_symbol(t_limit *x, t_symbol *s){
    x->x_selector = &s_symbol;
    x->x_symbol = s;
    limit_anything(x, x->x_selector, 0, 0);
}

static void limit_list(t_limit *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    x->x_selector = &s_list;
    limit_anything(x, x->x_selector, ac, av);
}

static void limit_free(t_limit *x){
    freebytes(x->x_message, x->x_size * sizeof(*x->x_message));
    if(x->x_clock)
        clock_free(x->x_clock);
}

static void *limit_new(t_floatarg f){
    t_limit *x = (t_limit *)pd_new(limit_class);
    x->x_open = 1;
    x->x_delta = f;
    x->x_selector = 0;
    x->x_float = 0;
    x->x_symbol = 0;
    x->x_size = INISIZE;
    x->x_natoms = 0;
    x->x_entered = 0;
    x->x_message = (t_atom *)getbytes(x->x_size*sizeof(t_atom));
    floatinlet_new(&x->x_obj, &x->x_delta);
    outlet_new((t_object *)x, &s_anything);
    x->x_clock = clock_new(x, (t_method)limit_tick);
    return(x);
}

void limit_setup(void){
    limit_class = class_new(gensym("limit"), (t_newmethod)limit_new,
        (t_method)limit_free, sizeof(t_limit), 0, A_DEFFLOAT, 0);
    class_addbang(limit_class, limit_bang);
    class_addfloat(limit_class, limit_float);
    class_addsymbol(limit_class, limit_symbol);
    class_addlist(limit_class, limit_list);
    class_addanything(limit_class, limit_anything);
}
