/**
 * Copyright 2019 Frank Duerr
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND ANY 
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TTY_H
#define TTY_H

#include <termios.h>

/**
 * Set terminal to cbreak mode:
 * - Canonical mode is disabled, i.e., characters are reported 
 *   character-by-character rather than line-by-line.
 * - Echoing of characters is disabled.
 * - Some special characters such as Ctrl-C (SIGINT) remain active, i.e.,
 *   they still will cause signals. These signals should be caught to 
 *   reset terminal such that terminal does not remain in cbreak mode.
 * 
 * @param fd file descriptor for tty.
 * @param saved old tty state (can be used by reset_tty() to reset tty later).
 * @return 0 on success; -1 on error.
 */
int tty_cbreak(int fd, struct termios *saved);

/**
 * Reset terminal to a previously saved state.
 *
 * @param fd file descriptor of terminal to reset.
 * @param save saved terminal state (see tty_cbreak()).
 * @return 0 on success; -1 on error.
 */
int tty_reset(int fd, const struct termios *saved);

#endif
