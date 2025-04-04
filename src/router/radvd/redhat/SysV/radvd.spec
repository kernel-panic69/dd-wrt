
%define initdir %{_sysconfdir}/rc.d/init.d

%define RADVD_UID 75

Summary: A Router Advertisement daemon
Name: radvd
Version: 2.20
Release: 1
# The code includes the advertising clause, so it's GPL-incompatible
License: BSD with advertising
Group: System Environment/Daemons
URL:        http://www.litech.org/radvd/
Source:     http://www.litech.org/radvd/dist/%{name}-%{version}.tar.gz
Requires(postun):   chkconfig, initscripts
Requires(preun):    chkconfig, initscripts
Requires(post):     chkconfig
Requires(pre):      /usr/sbin/useradd
# SuSE calls it libbsd0, Fedora calls it libbsd
Requires: libbsd0
BuildRequires: flex, byacc, libbsd-devel
BuildRoot:          %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
radvd is the router advertisement daemon for IPv6.  It listens to router
solicitations and sends router advertisements as described in "Neighbor
Discovery for IP Version 6 (IPv6)" (RFC 2461).  With these advertisements
hosts can automatically configure their addresses and some other
parameters.  They also can choose a default router based on these
advertisements.

Install radvd if you are setting up IPv6 network and/or Mobile IPv6
services.

%prep
%setup -q

%build
export CFLAGS="$RPM_OPT_FLAGS -D_GNU_SOURCE -fPIE -fno-strict-aliasing" 
export LDFLAGS='-pie -Wl,-z,relro,-z,now,-z,noexecstack,-z,nodlopen'
%configure --with-pidfile=%{_localstatedir}/run/radvd/radvd.pid
make
# make %{?_smp_mflags} 
# Parallel builds still fail because seds that transform y.tab.x into
# scanner/gram.x are not executed before compile of scanner/gram.x
#

%install
[ $RPM_BUILD_ROOT != "/" ] && rm -rf $RPM_BUILD_ROOT

make DESTDIR=$RPM_BUILD_ROOT install

mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig
mkdir -p $RPM_BUILD_ROOT%{initdir}
mkdir -p $RPM_BUILD_ROOT%{_localstatedir}/run/radvd

install -m 644 redhat/radvd.conf.empty $RPM_BUILD_ROOT%{_sysconfdir}/radvd.conf
install -m 755 redhat/radvd.init $RPM_BUILD_ROOT%{initdir}/radvd
install -m 644 redhat/radvd.sysconfig $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/radvd

install -d -m 755 $RPM_BUILD_ROOT%{_sysconfdir}/tmpfiles.d
install -p -m 644 redhat/radvd-tmpfs.conf $RPM_BUILD_ROOT%{_sysconfdir}/tmpfiles.d/radvd.conf 

%clean
[ $RPM_BUILD_ROOT != "/" ] && rm -rf $RPM_BUILD_ROOT

%postun
if [ "$1" -ge "1" ]; then
    /sbin/service radvd condrestart >/dev/null 2>&1
fi

%post
/sbin/chkconfig --add radvd

%preun
if [ $1 = 0 ]; then
   /sbin/service radvd stop >/dev/null 2>&1
   /sbin/chkconfig --del radvd
fi

%pre
getent group radvd >/dev/null || groupadd -g %RADVD_UID -r radvd
getent passwd radvd >/dev/null || \
  useradd -r -u %RADVD_UID -g radvd -d / -s /sbin/nologin -c "radvd user" radvd
exit 0

%files
%defattr(-,root,root,-)
%doc COPYRIGHT README CHANGES INTRO.html TODO
%config(noreplace) %{_sysconfdir}/radvd.conf
%config(noreplace) %{_sysconfdir}/sysconfig/radvd
%config(noreplace) %{_sysconfdir}/tmpfiles.d/radvd.conf
%{initdir}/radvd
%dir %attr(-,radvd,radvd) %{_localstatedir}/run/radvd/
%doc radvd.conf.example
%{_mandir}/*/*
%{_sbindir}/radvd
%{_sbindir}/radvdump
