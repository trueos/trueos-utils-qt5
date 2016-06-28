# Created by: Joe Maloney <jmaloney@ixsystems.com>
# $FreeBSD$

PORTNAME=	trueos-utils-qt5
PORTVERSION=	%%CHGVERSION%%
CATEGORIES=	sysutils

MAINTAINER=	jmaloney@ixsystems.com
COMMENT=	TrueOS QT5 Utilities

LICENSE=	BSD2CLAUSE

WRKSRC_SUBDIR=  src-qt5
MAKE_ARGS=      PREFIX=${STAGEDIR}${PREFIX}

CONFLICTS=      pcbsd-utils* 

MAKE_JOBS_UNSAFE=yes
ONLY_FOR_ARCHS= amd64
USE_LDCONFIG=   yes
USE_QT5=        core gui network svg linguist \
                buildtools x11extras
USES=           desktop-file-utils qmake shebangfix tar:xz

WRKSRC_SUBDIR=  src-qt5

QMAKE_ARGS=     CONFIG+="configure"

USE_GITHUB=	yes
GH_ACCOUNT=	trueos
GH_PROJECT=	trueos-utils-qt5
GH_TAGNAME=	%%GHTAG%%

.include <bsd.port.mk>
