/* JTox.c
 *
 *  Copyright (C) 2013 Tox project All Rights Reserved.
 *
 *  This file is part of jToxcore
 *
 *  jToxcore is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  jToxcore is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with jToxcore.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#endif
#include <tox/tox.h>

#include "JTox.h"
#include "callbacks.h"

#define ADDR_SIZE_HEX (TOX_FRIEND_ADDRESS_SIZE * 2 + 1)
#define UNUSED(x) (void)(x)

#ifdef ANDROID
#define ATTACH_THREAD(ptr,env) (*ptr->jvm)->AttachCurrentThread(ptr->jvm, &env, 0)
#else
#define ATTACH_THREAD(ptr,env) (*ptr->jvm)->AttachCurrentThread(ptr->jvm, (void **) &env, 0)
#endif

/**
 * Begin Utilities section
 */

/**
 * Convert a given binary address to a human-readable, \0-terminated hexadecimal string
 */
void addr_to_hex(uint8_t *addr, char *buf)
{
	uint32_t i;

	for (i = 0; i < TOX_FRIEND_ADDRESS_SIZE; i++) {
		char xx[3];
		snprintf(xx, sizeof(xx), "%02X", addr[i] & 0xff);
		strcat(buf, xx);
	}
}

/**
 * End Utilities section
 */

/**
 * Begin maintenance section
 */

JNIEXPORT jlong JNICALL Java_im_tox_jtoxcore_JTox_tox_1new(JNIEnv *env, jobject jobj)
{
	tox_jni_globals_t *globals = malloc(sizeof(tox_jni_globals_t));
	JavaVM *jvm;
	jclass clazz = (*env)->GetObjectClass(env, jobj);
	jfieldID id = (*env)->GetFieldID(env, clazz, "handler", "Lim/tox/jtoxcore/callbacks/CallbackHandler;");
	jobject handler = (*env)->GetObjectField(env, jobj, id);
	jobject handlerRef = (*env)->NewGlobalRef(env, handler);
	jobject jtoxRef = (*env)->NewGlobalRef(env, jobj);
	(*env)->GetJavaVM(env, &jvm);
	globals->tox = tox_new(1);
	globals->jvm = jvm;
	globals->handler = handlerRef;
	globals->jtox = jtoxRef;

	tox_callback_friend_action(globals->tox, callback_action, globals);

	tox_callback_connection_status(globals->tox, callback_connectionstatus, globals);

	tox_callback_friend_message(globals->tox, callback_friendmessage, globals);

	tox_callback_friend_request(globals->tox, callback_friendrequest, globals);

	tox_callback_name_change(globals->tox, callback_namechange, globals);

	tox_callback_read_receipt(globals->tox, callback_read_receipt, globals);

	tox_callback_status_message(globals->tox, callback_statusmessage, globals);

	tox_callback_user_status(globals->tox, callback_userstatus, globals);

	tox_callback_typing_change(globals->tox, callback_typingstatus, globals);

	return ((jlong) ((intptr_t) globals));
}

JNIEXPORT jint JNICALL Java_im_tox_jtoxcore_JTox_tox_1bootstrap_1from_1address(JNIEnv *env, jobject obj,
		jlong messenger, jstring ip, jint port, jbyteArray address)
{
	const char *_ip = (*env)->GetStringUTFChars(env, ip, 0);
	jbyte *_address = (*env)->GetByteArrayElements(env, address, 0);

	uint16_t _port = htons((uint16_t) port);

	jint result = tox_bootstrap_from_address(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, _ip, 1, _port,
				  (uint8_t *) _address);

	(*env)->ReleaseStringUTFChars(env, ip, _ip);
	(*env)->ReleaseByteArrayElements(env, address, _address, JNI_ABORT);

	UNUSED(obj);
	return result;
}

JNIEXPORT void JNICALL Java_im_tox_jtoxcore_JTox_tox_1do(JNIEnv *env, jobject obj, jlong messenger)
{
	tox_do(((tox_jni_globals_t *) ((intptr_t) messenger))->tox);
	UNUSED(env);
	UNUSED(obj);
}

JNIEXPORT jint JNICALL Java_im_tox_jtoxcore_JTox_tox_1isconnected(JNIEnv *env, jobject obj, jlong messenger)
{
	UNUSED(env);
	UNUSED(obj);
	return tox_isconnected(((tox_jni_globals_t *) ((intptr_t) messenger))->tox);
}

JNIEXPORT void JNICALL Java_im_tox_jtoxcore_JTox_tox_1kill(JNIEnv *env, jobject jobj, jlong messenger)
{
	tox_jni_globals_t *globals = (tox_jni_globals_t *) ((intptr_t) messenger);
	tox_kill(globals->tox);
	(*env)->DeleteGlobalRef(env, globals->handler);
	free(globals);
	UNUSED(jobj);
}

JNIEXPORT jbyteArray JNICALL Java_im_tox_jtoxcore_JTox_tox_1save(JNIEnv *env, jobject obj, jlong messenger)
{
	Tox *tox = ((tox_jni_globals_t *) ((intptr_t) messenger))->tox;
	uint32_t size = tox_size(tox);
	uint8_t *data = malloc(size);
	jbyteArray bytes = (*env)->NewByteArray(env, size);
	tox_save(tox, data);
	(*env)->SetByteArrayRegion(env, bytes, 0, size, (jbyte *) data);
	free(data);

	UNUSED(obj);
	return bytes;
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1load(JNIEnv *env, jobject obj, jlong messenger,
		jbyteArray bytes, jint length)
{
	jbyte *data = (*env)->GetByteArrayElements(env, bytes, 0);

	UNUSED(obj);
	return tox_load(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, (uint8_t *) data, length) == 0 ?
		   JNI_FALSE : JNI_TRUE;
}

/**
 * End maintenance section
 */

/**
 * Begin general section
 */

JNIEXPORT jint JNICALL Java_im_tox_jtoxcore_JTox_tox_1add_1friend(JNIEnv *env, jobject obj, jlong messenger,
		jbyteArray address, jbyteArray data, jint length)
{
	jbyte *_address = (*env)->GetByteArrayElements(env, address, 0);
	jbyte *_data = (*env)->GetByteArrayElements(env, data, 0);

	int ret = tox_add_friend(((tox_jni_globals_t *)((intptr_t)messenger))->tox, (uint8_t *) _address, (uint8_t *) _data,
							 length);

	(*env)->ReleaseByteArrayElements(env, address, _address, JNI_ABORT);
	(*env)->ReleaseByteArrayElements(env, data, _data, JNI_ABORT);

	UNUSED(obj);
	return ret;
}

JNIEXPORT jint JNICALL Java_im_tox_jtoxcore_JTox_tox_1add_1friend_1norequest(JNIEnv *env, jobject obj, jlong messenger,
		jbyteArray address)
{
	jbyte *_address = (*env)->GetByteArrayElements(env, address, 0);

	int ret = tox_add_friend_norequest(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, (uint8_t *) _address);
	(*env)->ReleaseByteArrayElements(env, address, _address, JNI_ABORT);

	UNUSED(obj);
	return ret;
}

JNIEXPORT jstring JNICALL Java_im_tox_jtoxcore_JTox_tox_1get_1address(JNIEnv *env, jobject obj, jlong messenger)
{
	jstring result;
	uint8_t addr[TOX_FRIEND_ADDRESS_SIZE];
	char id[ADDR_SIZE_HEX] = { 0 };
	tox_get_address(((tox_jni_globals_t *)((intptr_t) messenger))->tox, addr);
	addr_to_hex(addr, id);

	UNUSED(obj);
	result = (*env)->NewStringUTF(env, id);
	return result;
}

JNIEXPORT jstring JNICALL Java_im_tox_jtoxcore_JTox_tox_1get_1client_1id(JNIEnv *env, jobject obj, jlong messenger,
		jint friendnumber)
{
	uint8_t address[TOX_FRIEND_ADDRESS_SIZE];
	jstring result;
	UNUSED(obj);

	if (tox_get_client_id(((tox_jni_globals_t *)((intptr_t) messenger))->tox, friendnumber, address) != 0) {
		return 0;
	} else {
		char _address[ADDR_SIZE_HEX] = { 0 };
		addr_to_hex(address, _address);
		result = (*env)->NewStringUTF(env, _address);
		return result;
	}
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1del_1friend(JNIEnv *env, jobject obj, jlong messenger,
		jint friendnumber)
{
	UNUSED(env);
	UNUSED(obj);
	return tox_del_friend(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, friendnumber) == 0 ? 0 : 1;
}

JNIEXPORT jint JNICALL Java_im_tox_jtoxcore_JTox_tox_1send_1message(JNIEnv *env, jobject obj, jlong messenger,
		jint friendnumber, jbyteArray message, jint length)
{
	jbyte *_message = (*env)->GetByteArrayElements(env, message, 0);

	uint32_t mess_id = tox_send_message(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, friendnumber,
										(uint8_t *) _message,
										length);
	(*env)->ReleaseByteArrayElements(env, message, _message, JNI_ABORT);

	UNUSED(obj);
	return mess_id;
}

JNIEXPORT jint JNICALL Java_im_tox_jtoxcore_JTox_tox_1send_1message_1withid(JNIEnv *env, jobject obj, jlong messenger,
		jint friendnumber, jbyteArray message, jint length, jint messageID)
{
	jbyte *_message = (*env)->GetByteArrayElements(env, message, 0);

	uint32_t mess_id = tox_send_message_withid(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, friendnumber,
					   messageID, (uint8_t *) _message, length);
	(*env)->ReleaseByteArrayElements(env, message, _message, JNI_ABORT);

	UNUSED(obj);
	return mess_id;
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1send_1action(JNIEnv *env, jobject obj, jlong messenger,
		jint friendnumber, jbyteArray action, jint length)
{
	jbyte *_action = (*env)->GetByteArrayElements(env, action, 0);

	jboolean ret = tox_send_action(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, friendnumber, (uint8_t *) _action,
								   length);
	(*env)->ReleaseByteArrayElements(env, action, _action, JNI_ABORT);

	UNUSED(obj);
	return ret;
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1set_1name(JNIEnv *env, jobject obj, jlong messenger,
		jbyteArray newname, jint length)
{
	jbyte *_newname = (*env)->GetByteArrayElements(env, newname, 0);

	jboolean ret =
		tox_set_name(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, (uint8_t *) _newname, length) == 0 ?
		JNI_FALSE : JNI_TRUE;
	(*env)->ReleaseByteArrayElements(env, newname, _newname, JNI_ABORT);

	UNUSED(obj);
	return ret;
}

JNIEXPORT jstring JNICALL Java_im_tox_jtoxcore_JTox_tox_1get_1self_1name(JNIEnv *env, jobject obj, jlong messenger)
{
	jstring _name;
	uint8_t *name = malloc(TOX_MAX_NAME_LENGTH);
	uint16_t length = tox_get_self_name(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, name);

	if (length == 0) {
		free(name);
		return 0;
	}

	_name = (*env)->NewStringUTF(env, (char *) name);
	free(name);

	UNUSED(obj);
	return _name;
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1set_1status_1message(JNIEnv *env, jobject obj,
		jlong messenger, jbyteArray newstatus, jint length)
{
	jbyte *_newstatus = (*env)->GetByteArrayElements(env, newstatus, 0);
	jboolean ret =
		tox_set_status_message(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, (uint8_t *) _newstatus, length) == 0 ?
		JNI_FALSE :
		JNI_TRUE;
	(*env)->ReleaseByteArrayElements(env, newstatus, _newstatus, JNI_ABORT);

	UNUSED(obj);
	return ret;
}

JNIEXPORT jint JNICALL Java_im_tox_jtoxcore_JTox_tox_1get_1friend_1connection_1status(JNIEnv *env, jobject obj,
		jlong messenger, jint friendnumber)
{
	uint32_t ret = tox_get_friend_connection_status(((tox_jni_globals_t *)((intptr_t)messenger))->tox, friendnumber);

	UNUSED(env);
	UNUSED(obj);
	return ret;
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1get_1friend_1exists(JNIEnv *env, jobject obj, jlong messenger,
		jint friendnumber)
{
	uint8_t ret = tox_friend_exists(((tox_jni_globals_t *)((intptr_t)messenger))->tox, friendnumber);

	UNUSED(env);
	UNUSED(obj);
	return ret;
}

JNIEXPORT jbyteArray JNICALL Java_im_tox_jtoxcore_JTox_tox_1get_1name(JNIEnv *env, jobject obj, jlong messenger,
		jint friendnumber)
{
	jbyte *name = malloc(TOX_MAX_NAME_LENGTH);
	int ret = tox_get_name(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, friendnumber, (uint8_t *) name);

	UNUSED(obj);

	if (ret == -1) {
		free(name);
		return 0;
	} else {
		jbyteArray _name = (*env)->NewByteArray(env, ret);
		(*env)->SetByteArrayRegion(env, _name, 0, ret, name);
		free(name);
		return _name;
	}
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1set_1user_1status(JNIEnv *env, jobject obj, jlong messenger,
		jint userstatus)
{
	UNUSED(env);
	UNUSED(obj);

	return tox_set_user_status(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, userstatus) == 0 ?
		   JNI_FALSE : JNI_TRUE;
}

JNIEXPORT jbyteArray JNICALL Java_im_tox_jtoxcore_JTox_tox_1get_1status_1message(JNIEnv *env, jobject obj,
		jlong messenger, jint friendnumber)
{
	Tox *tox = ((tox_jni_globals_t *) ((intptr_t) messenger))->tox;
	int size = tox_get_status_message_size(tox, friendnumber);
	jbyte *statusmessage = malloc(size);
	int ret = tox_get_status_message(tox, friendnumber, (uint8_t *) statusmessage, size);

	UNUSED(obj);

	if (ret == -1) {
		free(statusmessage);
		return 0;
	} else {
		jbyteArray _statusmessage = (*env)->NewByteArray(env, ret);
		(*env)->SetByteArrayRegion(env, _statusmessage, 0, ret, statusmessage);
		free(statusmessage);
		return _statusmessage;
	}
}

JNIEXPORT jbyteArray JNICALL Java_im_tox_jtoxcore_JTox_tox_1getselfstatusmessage(JNIEnv *env, jobject obj,
		jlong messenger)
{
	Tox *tox = ((tox_jni_globals_t *) ((intptr_t) messenger))->tox;
	jbyte *status = malloc(TOX_MAX_STATUSMESSAGE_LENGTH);
	int length = tox_get_self_status_message(tox, (uint8_t *) status,
				 TOX_MAX_STATUSMESSAGE_LENGTH);

	UNUSED(obj);

	if (length == -1) {
		free(status);
		return 0;
	} else {
		jbyteArray _status = (*env)->NewByteArray(env, length);
		(*env)->SetByteArrayRegion(env, _status, 0, length, status);
		free(status);
		return _status;
	}
}

JNIEXPORT jobject JNICALL Java_im_tox_jtoxcore_JTox_tox_1get_1user_1status(JNIEnv *env, jobject obj, jlong messenger,
		jint friendnumber)
{
	Tox *tox = ((tox_jni_globals_t *) ((intptr_t) messenger))->tox;
	char *status;
	jclass us_enum;
	jfieldID fieldID;

	switch (tox_get_user_status(tox, friendnumber)) {
		case TOX_USERSTATUS_NONE:
			status = "TOX_USERSTATUS_NONE";
			break;

		case TOX_USERSTATUS_AWAY:
			status = "TOX_USERSTATUS_AWAY";
			break;

		case TOX_USERSTATUS_BUSY:
			status = "TOX_USERSTATUS_BUSY";
			break;

		default:
			status = "TOX_USERSTATUS_INVALID";
			break;
	}

	us_enum = (*env)->FindClass(env, "im/tox/jtoxcore/ToxUserStatus");
	fieldID = (*env)->GetStaticFieldID(env, us_enum, status, "Lim/tox/jtoxcore/ToxUserStatus;");

	UNUSED(obj);
	return (*env)->GetStaticObjectField(env, us_enum, fieldID);
}

JNIEXPORT jobject JNICALL Java_im_tox_jtoxcore_JTox_tox_1get_1self_1user_1status(JNIEnv *env, jobject obj,
		jlong messenger)
{
	Tox *tox = ((tox_jni_globals_t *) ((intptr_t) messenger))->tox;
	char *status;
	jclass us_enum;
	jfieldID fieldID;

	switch (tox_get_self_user_status(tox)) {
		case TOX_USERSTATUS_NONE:
			status = "TOX_USERSTATUS_NONE";
			break;

		case TOX_USERSTATUS_AWAY:
			status = "TOX_USERSTATUS_AWAY";
			break;

		case TOX_USERSTATUS_BUSY:
			status = "TOX_USERSTATUS_BUSY";
			break;

		default:
			status = "TOX_USERSTATUS_INVALID";
			break;
	}

	us_enum = (*env)->FindClass(env, "im/tox/jtoxcore/ToxUserStatus");
	fieldID = (*env)->GetStaticFieldID(env, us_enum, status, "Lim/tox/jtoxcore/ToxUserStatus;");

	UNUSED(obj);
	return (*env)->GetStaticObjectField(env, us_enum, fieldID);
}

JNIEXPORT void JNICALL Java_im_tox_jtoxcore_JTox_tox_1set_1sends_1receipts(JNIEnv *env, jobject obj, jlong messenger,
		jboolean send, jint friendnumber)
{
	UNUSED(env);
	UNUSED(obj);
	tox_set_sends_receipts(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, friendnumber, send);
}

JNIEXPORT jintArray JNICALL Java_im_tox_jtoxcore_JTox_tox_1get_1friendlist(JNIEnv *env, jobject obj, jlong messenger)
{
	Tox *tox = ((tox_jni_globals_t *) ((intptr_t) messenger))->tox;
	uint32_t length = tox_count_friendlist(tox);
	int *list = malloc(length);
	uint32_t actual_length = tox_get_friendlist(tox, list, length);
	jintArray arr = (*env)->NewIntArray(env, actual_length);
	(*env)->SetIntArrayRegion(env, arr, 0, actual_length, (jint *) list);
	free(list);

	UNUSED(obj);
	return arr;
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1set_1user_1is_1typing
(JNIEnv *env, jobject obj, jlong messenger, jint friendnumber, jboolean typing)
{
	Tox *tox = ((tox_jni_globals_t *)((intptr_t) messenger))->tox;
	uint8_t is_typing;

	if (typing == JNI_TRUE) {
		is_typing = 1;
	} else {
		is_typing = 0;
	}

	UNUSED(env);
	UNUSED(obj);
	return tox_set_user_is_typing(tox, friendnumber, is_typing) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1get_1is_1typing
  (JNIEnv *env, jobject obj, jlong messenger, jint friendnumber)
  {
  	Tox *tox = ((tox_jni_globals_t *) ((intptr_t) messenger))->tox;

  	uint8_t is_typing = tox_get_is_typing(tox, friendnumber);

  	UNUSED(env);
  	UNUSED(obj);
  	return is_typing == 1 ? JNI_TRUE : JNI_FALSE;
  }
/**
 * End general section
 */

/**
 * Begin Callback Section
 */

static void callback_friendrequest(Tox *tox, uint8_t *pubkey, uint8_t *message, uint16_t length, void *rptr)
{
	tox_jni_globals_t *ptr = (tox_jni_globals_t *) rptr;
	JNIEnv *env;
	jclass clazz;
	jmethodID meth;
	char buf[ADDR_SIZE_HEX] = { 0 };
	jstring _pubkey;
	jbyteArray _message;

	ATTACH_THREAD(ptr, env);
	clazz = (*env)->GetObjectClass(env, ptr->handler);
	meth = (*env)->GetMethodID(env, clazz, "onFriendRequest", "(Ljava/lang/String;[B)V");

	addr_to_hex(pubkey, buf);
	_pubkey = (*env)->NewStringUTF(env, buf);
	_message = (*env)->NewByteArray(env, length);
	(*env)->SetByteArrayRegion(env, _message, 0, length, (jbyte *) message);

	(*env)->CallVoidMethod(env, ptr->handler, meth, _pubkey, _message);
	UNUSED(tox);
}

static void callback_friendmessage(Tox *tox, int friendnumber, uint8_t *message, uint16_t length, void *rptr)
{
	tox_jni_globals_t *ptr = (tox_jni_globals_t *) rptr;
	JNIEnv *env;
	jclass class;
	jmethodID meth;
	jbyteArray _message;

	ATTACH_THREAD(ptr, env);
	class = (*env)->GetObjectClass(env, ptr->handler);
	meth = (*env)->GetMethodID(env, class, "onMessage", "(I[B)V");

	_message = (*env)->NewByteArray(env, length);
	(*env)->SetByteArrayRegion(env, _message, 0, length, (jbyte *) message);
	(*env)->CallVoidMethod(env, ptr->handler, meth, friendnumber, _message);

	UNUSED(tox);
}

static void callback_action(Tox *tox, int32_t friendnumber, uint8_t *action, uint16_t length, void *rptr)
{
	tox_jni_globals_t *ptr = (tox_jni_globals_t *) rptr;
	JNIEnv *env;
	jclass class;
	jmethodID meth;
	jbyteArray _action;

	ATTACH_THREAD(ptr, env);
	class = (*env)->GetObjectClass(env, ptr->handler);
	meth = (*env)->GetMethodID(env, class, "onAction", "(I[B)V");

	_action = (*env)->NewByteArray(env, length);
	(*env)->SetByteArrayRegion(env, _action, 0, length, (jbyte *) action);
	(*env)->CallVoidMethod(env, ptr->handler, meth, friendnumber, _action);

	UNUSED(tox);
}

static void callback_namechange(Tox *tox, int32_t friendnumber, uint8_t *newname, uint16_t length, void *rptr)
{
	tox_jni_globals_t *ptr = (tox_jni_globals_t *) rptr;
	JNIEnv *env;
	jclass handlerClass;
	jmethodID handlerMeth;

	jclass jtoxclass;
	jmethodID jToxmeth;
	jbyteArray _newname;

	ATTACH_THREAD(ptr, env);
	handlerClass = (*env)->GetObjectClass(env, ptr->handler);
	handlerMeth = (*env)->GetMethodID(env, handlerClass, "onNameChange", "(I[B)V");

	jtoxclass = (*env)->GetObjectClass(env, ptr->jtox);
	jToxmeth = (*env)->GetMethodID(env, jtoxclass, "onNameChange", "(I[B)V");

	_newname = (*env)->NewByteArray(env, length);
	(*env)->SetByteArrayRegion(env, _newname, 0, length, (jbyte *) newname);
	(*env)->CallVoidMethod(env, ptr->jtox, jToxmeth, friendnumber, _newname);
	(*env)->CallVoidMethod(env, ptr->handler, handlerMeth, friendnumber, _newname);

	UNUSED(tox);
}

static void callback_statusmessage(Tox *tox, int32_t friendnumber, uint8_t *newstatus, uint16_t length, void *rptr)
{
	tox_jni_globals_t *ptr = (tox_jni_globals_t *) rptr;
	JNIEnv *env;
	jclass handlerclass;
	jmethodID handlermeth;

	jclass jtoxclass;
	jmethodID jtoxmeth;
	jbyteArray _newstatus;

	ATTACH_THREAD(ptr, env);
	handlerclass = (*env)->GetObjectClass(env, ptr->handler);
	handlermeth = (*env)->GetMethodID(env, handlerclass, "onStatusMessage", "(I[B)V");
	jtoxclass = (*env)->GetObjectClass(env, ptr->jtox);
	jtoxmeth = (*env)->GetMethodID(env, jtoxclass, "onStatusMessage", "(I[B)V");

	_newstatus = (*env)->NewByteArray(env, length);
	(*env)->SetByteArrayRegion(env, _newstatus, 0, length, (jbyte *) newstatus);
	(*env)->CallVoidMethod(env, ptr->jtox, jtoxmeth, friendnumber, _newstatus);
	(*env)->CallVoidMethod(env, ptr->handler, handlermeth, friendnumber, _newstatus);

	UNUSED(tox);
}

static void callback_userstatus(Tox *tox, int32_t friendnumber, uint8_t status, void *rptr)
{
	tox_jni_globals_t *ptr = (tox_jni_globals_t *) rptr;
	JNIEnv *env;
	jclass handlerclass;
	jmethodID handlermeth;

	jclass jtoxclass;
	jmethodID jtoxmeth;

	jclass us_enum;
	char *enum_name;

	jfieldID fieldID;
	jobject enum_val;

	ATTACH_THREAD(ptr, env);
	handlerclass = (*env)->GetObjectClass(env, ptr->handler);
	handlermeth = (*env)->GetMethodID(env, handlerclass, "onUserStatus",
									  "(ILim/tox/jtoxcore/ToxUserStatus;)V");
	jtoxclass = (*env)->GetObjectClass(env, ptr->jtox);
	jtoxmeth = (*env)->GetMethodID(env, jtoxclass, "onUserStatus", "(ILim/tox/jtoxcore/ToxUserStatus;)V");
	us_enum = (*env)->FindClass(env, "im/tox/jtoxcore/ToxUserStatus");

	switch (status) {
		case TOX_USERSTATUS_NONE:
			enum_name = "TOX_USERSTATUS_NONE";
			break;

		case TOX_USERSTATUS_AWAY:
			enum_name = "TOX_USERSTATUS_AWAY";
			break;

		case TOX_USERSTATUS_BUSY:
			enum_name = "TOX_USERSTATUS_BUSY";
			break;

		default:
			enum_name = "TOX_USERSTATUS_INVALID";
			break;
	}

	fieldID = (*env)->GetStaticFieldID(env, us_enum, enum_name, "Lim/tox/jtoxcore/ToxUserStatus;");
	enum_val = (*env)->GetStaticObjectField(env, us_enum, fieldID);
	(*env)->CallVoidMethod(env, ptr->jtox, jtoxmeth, friendnumber, enum_val);
	(*env)->CallVoidMethod(env, ptr->handler, handlermeth, friendnumber, enum_val);

	UNUSED(tox);
}

static void callback_read_receipt(Tox *tox, int32_t friendnumber, uint32_t receipt, void *rptr)
{
	tox_jni_globals_t *ptr = (tox_jni_globals_t *) rptr;
	JNIEnv *env;
	jclass handlerclass;
	jmethodID handlermeth;

	ATTACH_THREAD(ptr, env);
	handlerclass = (*env)->GetObjectClass(env, ptr->handler);
	handlermeth = (*env)->GetMethodID(env, handlerclass, "onReadReceipt", "(II)V");
	(*env)->CallVoidMethod(env, ptr->handler, handlermeth, friendnumber, receipt);

	UNUSED(tox);
}

static void callback_connectionstatus(Tox *tox, int32_t friendnumber, uint8_t newstatus, void *rptr)
{
	tox_jni_globals_t *ptr = (tox_jni_globals_t *) rptr;
	JNIEnv *env;
	jclass handlerclass;
	jmethodID handlermeth;

	jclass jtoxclass;
	jmethodID jtoxmeth;

	jboolean _newstatus;

	ATTACH_THREAD(ptr, env);
	handlerclass = (*env)->GetObjectClass(env, ptr->handler);
	handlermeth = (*env)->GetMethodID(env, handlerclass, "onConnectionStatus", "(IZ)V");
	jtoxclass = (*env)->GetObjectClass(env, ptr->jtox);
	jtoxmeth = (*env)->GetMethodID(env, jtoxclass, "onConnectionStatus", "(IZ)V");
	_newstatus = newstatus == 0 ? JNI_FALSE : JNI_TRUE;
	(*env)->CallVoidMethod(env, ptr->jtox, jtoxmeth, friendnumber, _newstatus);
	(*env)->CallVoidMethod(env, ptr->handler, handlermeth, friendnumber, _newstatus);

	UNUSED(tox);
}

static void callback_typingstatus(Tox *tox, int32_t friendnumber, uint8_t is_typing, void *rptr)
{
	tox_jni_globals_t *ptr = (tox_jni_globals_t *) rptr;
	JNIEnv *env;
	jclass handlerclass;
	jmethodID handlermeth;

	jclass jtoxclass;
	jmethodID jtoxmeth;

	jboolean _is_typing;

	ATTACH_THREAD(ptr, env);
	handlerclass = (*env)->GetObjectClass(env, ptr->handler);
	handlermeth = (*env)->GetMethodID(env, handlerclass, "onTypingChange", "(IZ)V");
	jtoxclass = (*env)->GetObjectClass(env, ptr->jtox);
	jtoxmeth = (*env)->GetMethodID(env, jtoxclass, "onTypingChange", "(IZ)V");
	_is_typing = is_typing == 0 ? JNI_FALSE : JNI_TRUE;
	(*env)->CallVoidMethod(env, ptr->jtox, jtoxmeth, friendnumber, _is_typing);
	(*env)->CallVoidMethod(env, ptr->handler, handlermeth, friendnumber, _is_typing);

	UNUSED(tox);
}
