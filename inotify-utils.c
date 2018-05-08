#include "./inotify-utils.h"

void fail(const char *message) {
	perror(message);
	exit(1);
}

const char * target_type(struct inotify_event *event) {
	if( event->len == 0 )
		return "";
	else
		return event->mask & IN_ISDIR ? "directory" : "file";
}

const char * target_name(struct inotify_event *event) {
	return event->len > 0 ? event->name : NULL;
}

const char * event_name(struct inotify_event *event) {
	if (event->mask & IN_ACCESS)
		return "access";
	else if (event->mask & IN_ATTRIB)
		return "attrib";
	else if (event->mask & IN_CLOSE_WRITE)
		return "close write";
	else if (event->mask & IN_CLOSE_NOWRITE)
		return "close nowrite";
	else if (event->mask & IN_CREATE)
		return "create";
	else if (event->mask & IN_DELETE)
		return "delete";
	else if (event->mask & IN_DELETE_SELF)
		return "watch target deleted";
	else if (event->mask & IN_MODIFY)
		return "modify";
	else if (event->mask & IN_MOVE_SELF)
		return "watch target moved";
	else if (event->mask & IN_MOVED_FROM)
		return "moved out";
	else if (event->mask & IN_MOVED_TO)
		return "moved into";
	else if (event->mask & IN_OPEN)
		return "open";
	else
		return "unknown event";
}
