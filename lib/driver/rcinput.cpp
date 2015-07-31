#include <lib/driver/rcinput.h>

#include <lib/base/eerror.h>

#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/kd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <lib/base/ebase.h>
#include <lib/base/init.h>
#include <lib/base/init_num.h>
#include <lib/driver/input_fake.h>

static bool bflag;

void eRCDeviceInputDev::handleCode(long rccode)
{
	struct input_event *ev = (struct input_event *)rccode;

#if WETEKRC
/*
	eDebug("==> BEFORE check for evtype: %x %x %x", ev->value, ev->code, ev->type);
	eDebug("==> BEFORE check for evtype:-->BackspaceFLAG %d", bflag);
*/
	if (ev->code == KEY_BACKSPACE && ev->value == 1 ) {
		bflag = !bflag;
	}
/*
	eDebug("==> BEFORE check for evtype after check for evvalue:-->BackspaceFLAG %d", bflag);
*/
#endif

	if (ev->type != EV_KEY)
		return;

	eDebug("%x %x %x", ev->value, ev->code, ev->type);

	int km = iskeyboard ? input->getKeyboardMode() : eRCInput::kmNone;

	switch (ev->code)
	{
		case KEY_LEFTSHIFT:
		case KEY_RIGHTSHIFT:
			shiftState = ev->value;
			break;
		case KEY_CAPSLOCK:
			if (ev->value == 1)
				capsState = !capsState;
			break;
	}

	if (km == eRCInput::kmAll)
		return;

	if (km == eRCInput::kmAscii)
	{
		bool ignore = false;
		bool ascii = (ev->code > 0 && ev->code < 61);

		switch (ev->code)
		{
			case KEY_LEFTCTRL:
			case KEY_RIGHTCTRL:
			case KEY_LEFTSHIFT:
			case KEY_RIGHTSHIFT:
			case KEY_LEFTALT:
			case KEY_RIGHTALT:
			case KEY_CAPSLOCK:
				ignore = true;
				break;
			case KEY_RESERVED:
			case KEY_ESC:
			case KEY_TAB:
			case KEY_BACKSPACE:
/*
				bflag = !bflag;
				eDebug("--> AFTER flip BackspaceFLAG %d", bflag);
*/
			case KEY_ENTER:
			case KEY_INSERT:
			case KEY_DELETE:
			case KEY_MUTE:
				ascii = false;
			default:
				break;
		}

		if (ignore)
			return;

		if (ascii)
		{
			if (ev->value)
			{
				if (consoleFd >= 0)
				{
					struct kbentry ke;
					/* off course caps is not the same as shift, but this will have to do for now */
					ke.kb_table = (shiftState || capsState) ? K_SHIFTTAB : K_NORMTAB;
					ke.kb_index = ev->code;
					::ioctl(consoleFd, KDGKBENT, &ke);
					if (ke.kb_value)
						input->keyPressed(eRCKey(this, ke.kb_value & 0xff, eRCKey::flagAscii)); /* emit */ 
				}
			}
			return;
		}
	}

#if WETEKRC
/*
	eDebug("-->BackspaceFLAG %d", bflag);
	eDebug("-->before change %x %x %x", ev->value, ev->code, ev->type);
*/
/* default is with NO numerc keys !!!*/
	if (bflag) {
		if (ev->code == KEY_1) {
			ev->code = KEY_RED;
		}
		if (ev->code == KEY_2) {
			ev->code = KEY_GREEN;
		}
		if (ev->code == KEY_3) {
			ev->code = KEY_YELLOW;
		}
		if (ev->code == KEY_4) {
			ev->code = KEY_BLUE;
		}
		if (ev->code == KEY_5) {
			ev->code = KEY_PREVIOUS;
		}
		if (ev->code == KEY_6) {
			ev->code = KEY_NEXT;
		}
		if (ev->code == KEY_7) {
			ev->code = KEY_REWIND;
		}
		if (ev->code == KEY_8) {
			ev->code = KEY_STOP;
		}
		if (ev->code == KEY_9) {
			ev->code = KEY_FASTFORWARD;
		}
		if (ev->code == KEY_0) {
			ev->code = KEY_PLAYPAUSE;
		}
	}
/*
	eDebug("-->BackspaceFLAG %d", bflag);
	eDebug("-->after change %x %x %x", ev->value, ev->code, ev->type);
*/
#endif

#if KEY_F7_TO_KEY_MENU
	if (ev->code == KEY_F7) {
		ev->code = KEY_MENU;
	}
#endif

#if KEY_F1_TO_KEY_MEDIA
	if (ev->code == KEY_F1) {
		ev->code = KEY_MEDIA;
	}
#endif

#if KEY_HOME_TO_KEY_INFO
	if (ev->code == KEY_HOME) {
		ev->code = KEY_INFO;
	}
#endif

#if KEY_BACK_TO_KEY_EXIT
	if (ev->code == KEY_BACK) {
		ev->code = KEY_EXIT;
	}
#endif

#if KEY_F2_TO_KEY_EPG
	if (ev->code == KEY_F2) {
		ev->code = KEY_EPG;
	}
#endif

#if KEY_ENTER_TO_KEY_OK
	if (ev->code == KEY_ENTER) {
		ev->code = KEY_OK;
	}
#endif

#if KEY_TEXT_TO_KEY_AUDIO
	if (ev->code == KEY_AUDIO)
	{
		/* AZBOX rc has a KEY aux key, which sends KEY_TEXT events. Correct this, so we do not have to place hacks in the keymaps. */
		ev->code = KEY_TEXT;
	}
	else if (ev->code == KEY_AUDIO)
	{
		/* AZBOX rc has a KEY Check key, which sends KEY_AUDIO events. Correct this, so we do not have to place hacks in the keymaps. */
		ev->code = KEY_TEXT;
	}
#endif

#if KEY_PLAY_ACTUALLY_IS_KEY_PLAYPAUSE
	if (ev->code == KEY_PLAY)
	{
		if (id == "dreambox advanced remote control (native)")
		{
			/* 8k rc has a KEY_PLAYPAUSE key, which sends KEY_PLAY events. Correct this, so we do not have to place hacks in the keymaps. */
			ev->code = KEY_PLAYPAUSE;
		}
	}
#endif

	switch (ev->value)
	{
		case 0:
			input->keyPressed(eRCKey(this, ev->code, eRCKey::flagBreak)); /*emit*/ 
			break;
		case 1:
			input->keyPressed(eRCKey(this, ev->code, 0)); /*emit*/ 
			break;
		case 2:
			input->keyPressed(eRCKey(this, ev->code, eRCKey::flagRepeat)); /*emit*/ 
			break;
	}
}

eRCDeviceInputDev::eRCDeviceInputDev(eRCInputEventDriver *driver, int consolefd)
	:	eRCDevice(driver->getDeviceName(), driver), iskeyboard(driver->isKeyboard()),
		ismouse(driver->isPointerDevice()),
		consoleFd(consolefd), shiftState(false), capsState(false)
{
	setExclusive(true);
	eDebug("Input device \"%s\" is a %s", id.c_str(), iskeyboard ? "keyboard" : (ismouse ? "mouse" : "remotecontrol"));
}

void eRCDeviceInputDev::setExclusive(bool b)
{
	if (!iskeyboard && !ismouse)
		driver->setExclusive(b);
}

const char *eRCDeviceInputDev::getDescription() const
{
	return id.c_str();
}

class eInputDeviceInit
{
	struct element
	{
		public:
			char* filename;
			eRCInputEventDriver* driver;
			eRCDeviceInputDev* device;
			element(const char* fn, eRCInputEventDriver* drv, eRCDeviceInputDev* dev):
				filename(strdup(fn)),
				driver(drv),
				device(dev)
			{
			}
			~element()
			{
				delete device;
				delete driver;
				free(filename);
			}
		private:
			element(const element& other); /* no copy */
	};
	typedef std::vector<element*> itemlist;
	std::vector<element*> items;
	int consoleFd;

public:
	eInputDeviceInit()
	{
		int i = 0;
		consoleFd = ::open("/dev/tty0", O_RDWR);
		while (1)
		{
			char filename[32];
			sprintf(filename, "/dev/input/event%d", i);
			if (::access(filename, R_OK) < 0) break;
			add(filename);
			++i;
		}
		eDebug("Found %d input devices.", i);
	}
	
	~eInputDeviceInit()
	{
		for (itemlist::iterator it = items.begin(); it != items.end(); ++it)
			delete *it;

		if (consoleFd >= 0)
			::close(consoleFd);
	}

	void add(const char* filename)
	{
		eRCInputEventDriver *p = new eRCInputEventDriver(filename);
		items.push_back(new element(filename, p, new eRCDeviceInputDev(p, consoleFd)));
	}

	void remove(const char* filename)
	{
		for (itemlist::iterator it = items.begin(); it != items.end(); ++it)
		{
			if (strcmp((*it)->filename, filename) == 0)
			{
				delete *it;
				items.erase(it);
				return;
			}
		}
		eDebug("Remove '%s', not found", filename);
	}
};

eAutoInitP0<eInputDeviceInit> init_rcinputdev(eAutoInitNumbers::rc+1, "input device driver");

void addInputDevice(const char* filename)
{
	init_rcinputdev->add(filename);
}

void removeInputDevice(const char* filename)
{
	init_rcinputdev->remove(filename);
}
