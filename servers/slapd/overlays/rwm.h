/* rwm.h - dn rewrite/attribute mapping header file */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2004 The OpenLDAP Foundation.
 * Portions Copyright 1999-2003 Howard Chu.
 * Portions Copyright 2000-2003 Pierangelo Masarati.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software and subsequently enhanced by Pierangelo
 * Masarati.
 */

#ifndef RWM_H
#define RWM_H

/* String rewrite library */
#ifdef ENABLE_REWRITE
#include "rewrite.h"
#endif /* ENABLE_REWRITE */

LDAP_BEGIN_DECL

struct ldapmap {
	int drop_missing;

	Avlnode *map;
	Avlnode *remap;
};

struct ldapmapping {
	int			m_flags;
#define	RWMMAP_F_NONE		0x00
#define	RWMMAP_F_IS_OC		0x01
#define RWMMAP_F_FREE_SRC	0x10
#define RWMMAP_F_FREE_DST	0x20
	struct berval		m_src;
	union {
		AttributeDescription	*m_s_ad;
		ObjectClass		*m_s_oc;
	} m_src_ref;
#define m_src_ad	m_src_ref.m_s_ad
#define m_src_oc	m_src_ref.m_s_oc
	struct berval		m_dst;
	union {
		AttributeDescription	*m_d_ad;
		ObjectClass		*m_d_oc;
	} m_dst_ref;
#define m_dst_ad	m_dst_ref.m_d_ad
#define m_dst_oc	m_dst_ref.m_d_oc
};

struct ldaprwmap {
	/*
	 * DN rewriting
	 */
#ifdef ENABLE_REWRITE
	struct rewrite_info *rwm_rw;
#else /* !ENABLE_REWRITE */
	/* some time the suffix massaging without librewrite
	 * will be disabled */
	BerVarray rwm_suffix_massage;
#endif /* !ENABLE_REWRITE */

	/*
	 * Attribute/objectClass mapping
	 */
	struct ldapmap rwm_oc;
	struct ldapmap rwm_at;
};

/* Whatever context ldap_back_dn_massage needs... */
typedef struct dncookie {
	struct ldaprwmap *rwmap;

#ifdef ENABLE_REWRITE
	Connection *conn;
	char *ctx;
	SlapReply *rs;
#else
	int normalized;
	int tofrom;
#endif
} dncookie;

int rwm_dn_massage(dncookie *dc, struct berval *dn, struct berval *res);

/* attributeType/objectClass mapping */
int rwm_mapping_cmp (const void *, const void *);
int rwm_mapping_dup (void *, void *);

void rwm_map_init ( struct ldapmap *lm, struct ldapmapping ** );
void rwm_map ( struct ldapmap *map, struct berval *s, struct berval *m,
	int remap );
#define RWM_MAP	0
#define RWM_REMAP	1
char *
rwm_map_filter(
		struct ldapmap *at_map,
		struct ldapmap *oc_map,
		struct berval *f,
		int remap
);

int
rwm_map_attrs(
		struct ldapmap *at_map,
		AttributeName *a,
		int remap,
		char ***mapped_attrs
);

extern void rwm_mapping_free ( void *mapping );

extern int rwm_map_config(
		struct ldapmap	*oc_map,
		struct ldapmap	*at_map,
		const char	*fname,
		int		lineno,
		int		argc,
		char		**argv );

extern int
rwm_filter_map_rewrite(
		dncookie		*dc,
		Filter			*f,
		struct berval		*fstr,
		int			remap );

/* suffix massaging by means of librewrite */
#ifdef ENABLE_REWRITE
extern int rwm_suffix_massage_config( struct rewrite_info *info,
		struct berval *pvnc, struct berval *nvnc,
		struct berval *prnc, struct berval *nrnc);
#endif /* ENABLE_REWRITE */
extern int rwm_dnattr_rewrite(
	Operation		*op,
	SlapReply		*rs,
	void			*cookie,
	BerVarray		a_vals
	);
extern int rwm_dnattr_result_rewrite( dncookie *dc, BerVarray a_vals );

LDAP_END_DECL

#endif /* RWM_H */
