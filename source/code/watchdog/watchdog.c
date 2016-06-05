/**
    Watchdog Max Object
    Stephen Hensley
    shensley.audio@gmail.com

    This object is in its early version. It is being adapted from the dummy object included in the Max SDK.

    Previous credits:

	@file
	dummy - a dummy object
	jeremy bernstein - jeremy@bootsquad.com

	@ingroup	examples
*/
#include <time.h>
#include "ext.h"							// standard Max include, always required
#include "ext_obex.h"						// required for new style Max object
#include "PwrUSBImp.h"

////////////////////////// object struct
typedef struct _watchdog
{
	t_object	ob;
	t_atom		val;
	t_symbol	*name;
    t_datetime  date;
    char        dateString[50];
    long        pulseSeconds;
    long        pulseMisses;
    long        resetTimeSec;

	void		*out;
} t_watchdog;

///////////////////////// function prototypes
//// standard set
void *watchdog_new(t_symbol *s, long argc, t_atom *argv);
void watchdog_free(t_watchdog *x);
void watchdog_assist(t_watchdog *x, void *b, long m, long a, char *s);

void watchdog_int(t_watchdog *x, long n);
void watchdog_float(t_watchdog *x, double f);
void watchdog_anything(t_watchdog *x, t_symbol *s, long ac, t_atom *av);
void watchdog_bang(t_watchdog *x);
void watchdog_identify(t_watchdog *x);
void watchdog_dblclick(t_watchdog *x);
void watchdog_acant(t_watchdog *x);

void watchdog_outputTime(t_watchdog *x);
void watchdog_startWatchdog(t_watchdog *x,long argCount,t_atom *argv);

//////////////////////// global class pointer variable
void *dummy_class;
int m_pwrUSBInit = 0;


void ext_main(void *r)
{
	t_class *c;
    
    
	c = class_new("watchdog", (method)watchdog_new, (method)watchdog_free, (long)sizeof(t_watchdog),
				  0L /* leave NULL!! */, A_GIMME, 0);

	class_addmethod(c, (method)watchdog_bang,			"bang", 0);
	class_addmethod(c, (method)watchdog_int,			"int",		A_LONG, 0);
	class_addmethod(c, (method)watchdog_float,			"float",	A_FLOAT, 0);
	class_addmethod(c, (method)watchdog_anything,		"anything",	A_GIMME, 0);
	class_addmethod(c, (method)watchdog_identify,		"identify", 0);
	CLASS_METHOD_ATTR_PARSE(c, "identify", "undocumented", gensym("long"), 0, "1");

	// we want to 'reveal' the otherwise hidden 'xyzzy' method
	class_addmethod(c, (method)watchdog_anything,		"start/stop", A_GIMME, 0);
	// here's an otherwise undocumented method, which does something that the user can't actually
	// do from the patcher however, we want them to know about it for some weird documentation reason.
	// so let's make it documentable. it won't appear in the quickref, because we can't send it from a message.
	class_addmethod(c, (method)watchdog_acant,			"blooop", A_CANT, 0);
	CLASS_METHOD_ATTR_PARSE(c, "blooop", "documentable", gensym("long"), 0, "1");

	/* you CAN'T call this from the patcher */
	class_addmethod(c, (method)watchdog_assist,			"assist",		A_CANT, 0);
	class_addmethod(c, (method)watchdog_dblclick,			"dblclick",		A_CANT, 0);

	CLASS_ATTR_SYM(c, "name", 0, t_watchdog, name);

	class_register(CLASS_BOX, c);
	dummy_class = c;
}

void watchdog_acant(t_watchdog *x)
{
	object_post((t_object *)x, "can't touch this!");
}

void watchdog_assist(t_watchdog *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) { //inlet
		sprintf(s, "I am inlet %ld, send me start/stop messages. identify for more info and a bang to send a heartbeat!", a);
	}
	else {	// outlet
		sprintf(s, "I am outlet %ld", a);
	}
}

void watchdog_free(t_watchdog *x)
{
	;
}

void watchdog_dblclick(t_watchdog *x)
{
	object_post((t_object *)x, "I got a double-click");
}

void watchdog_int(t_watchdog *x, long n)
{
	atom_setlong(&x->val, n);
	//dummy_bang(x);
}

void watchdog_float(t_watchdog *x, double f)
{
	atom_setfloat(&x->val, f);
	//dummy_bang(x);
}

void watchdog_anything(t_watchdog *x, t_symbol *s, long ac, t_atom *av)
{
    int success = 0;
	if( s == gensym("start")){
        if (CheckStatusPowerUSB()>0) {
            //start watchdog timer.
            
            int status = GetWatchdogStatusPowerUSB();
            if(status == 0){
                watchdog_startWatchdog(x,ac,av);
            }else if(status == 1){
                object_error((t_object *)x, "The Watchdog timer is already running.");
            }else if(status == 2){
                object_error((t_object *)x, "The Watchdog Device is restarting.");
            }else{
                object_post((t_object *)x, "Unrecocgnized Status Code: %d - Trying to start timer.",status);
                watchdog_startWatchdog(x,ac,av);
            }
        }else{
            error("There is no PowerUSB Device attached");
        }
    }else if(s == gensym("stop")){
         if (CheckStatusPowerUSB()>0) {
         //stop watchdog timer.
             int status = GetWatchdogStatusPowerUSB();
             if(status == 1){
                 success = StopWatchdogTimerPowerUSB();
                 if (success) {
                     object_post((t_object *) x, "The Watchdog has been stopped!");
                 }else{
                     error("Something went wrong, the timer was not started. There is %d powerUSB device connected.",CheckStatusPowerUSB());
                 }
             }else if(status == 0){
                 object_error((t_object *)x, "The Watchdog timer is already stopped.");
             }else if(status == 2){
                 object_error((t_object *)x, "The Watchdog Device is restarting.");
             }else{
                 object_error((t_object *)x, "Unrecocgnized Status Code: %d - Trying to stop timer.",status);
                 success = StopWatchdogTimerPowerUSB();
                 if (success) {
                     object_post((t_object *) x, "The Watchdog has been stopped!");
                 }else{
                     error("Something went wrong, the timer was not started. There is %d powerUSB device connected.",CheckStatusPowerUSB());
                 }
             }
         }else{
             error("There is no PowerUSB Device attached");
         }
    }else if(s == gensym("reset")) {
        if (CheckStatusPowerUSB()>0) {
            ResetBoard();
            post("PowerUSB Device Reset");
        }
    }else if(s == gensym("port")){
        int portStatus[3];
        ReadPortStatePowerUSB(&portStatus[0], &portStatus[1], &portStatus[2]);
        if(ac == 0){
            object_post((t_object *)x, "Port 1: %d, Port 2: %d, Port 3: %d",portStatus[0],portStatus[1],portStatus[2]);
        }else if(ac == 3){
            for (int i = 0; i < 3; i++) {
                portStatus[i] = atom_getlong(av+i);
            }
            SetPortPowerUSB(portStatus[0],portStatus[1],portStatus[2]);
            object_post((t_object *)x, "ports set: %d, %d, %d",portStatus[0],portStatus[1],portStatus[2]);
        }else{
            object_error((t_object *)x, "Call 'port' for port status, or call port x y z to set the port status for outlets 1, 2 and 3 respectively.");
        }
    }else{
		atom_setsym(&x->val, s);
		watchdog_bang(x);
	}
}

void watchdog_bang(t_watchdog *x)
{
    //int test = 1;
     if (CheckStatusPowerUSB()>0) {
     //heartbeat.
         SendHeartBeatPowerUSB();
     }else{
         error("There is no PowerUSB Device attached");
     }
    systime_datetime(&x->date);
    sysdateformat_formatdatetime(&x->date, SYSDATEFORMAT_FLAGS_LONG, 0, x->dateString, 50);
    outlet_anything(x->out,gensym(x->dateString),0,NULL);
    //dummy_outputTime(x);

}

void watchdog_identify(t_watchdog *x)
{
    char fw[8];
    int model;
    int count;
    if ((count = InitPowerUSB(&model,fw))>0) {
        m_pwrUSBInit = 1;
        object_post((t_object *)x,"Models: 1 - Basic, 2 - DigiIO, 3 - watchdog, 4 - smart");
        object_post((t_object *)x,"Model: %d, Firmware: %s, Number of Devices:%d",model,fw,count);
    }
    
    object_post((t_object *)x, "The timer will check for a heartbeat every %d seconds.\nIf the timer doesn't receive %d pulses it will reset the watchdog outlet for %d seconds.",x->pulseSeconds,x->pulseMisses,x->resetTimeSec);
}

void *watchdog_new(t_symbol *s, long argc, t_atom *argv)
{
	t_watchdog *x = NULL;
    int ret = -1;
    int model = -1;
    char fw[32];
    
    
    
    if (m_pwrUSBInit == 0) {
        if((ret = InitPowerUSB(&model, fw)) > 0){
            m_pwrUSBInit = 1;
            SetDefaultStatePowerUSB(1,1,1);
            SetPortPowerUSB(1,1,1);
        }
    }
	if ((x = (t_watchdog *)object_alloc(dummy_class))) {
		x->name = gensym("");
		if (argc && argv) {
			x->pulseSeconds = atom_getlong(argv);
            if(argv+1){
                x->pulseMisses = atom_getlong(argv+1);
            }else{
                x->pulseMisses = 5;
            }
            if (argv+2) {
                x->resetTimeSec = atom_getlong(argv+2);
            }else{
                x->resetTimeSec = 60;
            }
        }else{
            x->pulseSeconds = 30;
            x->pulseMisses = 5;
            x->resetTimeSec = 60;
        }
		if (!x->name || x->name == gensym(""))
			x->name = symbol_unique();
        
		atom_setlong(&x->val, 0);
		x->out = outlet_new(x, NULL);
	}
	return (x);
}


void watchdog_startWatchdog(t_watchdog *x, long argCount, t_atom *argv){
    int hbSec,numMis,resetSec,success;
    if (argCount == 3) {
        hbSec = argv;
        numMis = argv+1;
        resetSec = argv+2;
    }else{
        hbSec = x->pulseSeconds;
        numMis = x->pulseMisses;
        resetSec = x->resetTimeSec;
    }
    success = StartWatchdogTimerPowerUSB(hbSec,numMis,resetSec);
    
    if (success) {
        object_post((t_object *)x, "The Watchdog Timer has Started! The timer checks for heartbeats evert %d seconds. After missing %d heartbeats, the computer will powercycle for %d seconds.",hbSec,numMis,resetSec);
    }else{
        object_error((t_object *)x, "Something went wrong, the timer was not started. There is %d powerUSB device connected.",CheckStatusPowerUSB());
    }
}





