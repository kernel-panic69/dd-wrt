%define RADVD_UID 75

Summary: A Router Advertisement daemon
Name: radvd
Version: 2.20
Release: 1%{?dist}
# The code includes the advertising clause, so it's GPL-incompatible
License: BSD with advertising
Group: System Environment/Daemons
URL:        http://www.litech.org/radvd/
Source:     http://www.litech.org/radvd/dist/%{name}-%{version}.tar.gz

BuildRequires: gcc
BuildRequires: bison
BuildRequires: flex
BuildRequires: flex-static
BuildRequires: pkgconfig
BuildRequires: check-devel
BuildRequires: systemd
BuildRequires: libbsd-devel
%{?systemd_requires}
Requires(pre): shadow-utils
# SuSE calls it libbsd0, Fedora calls it libbsd
Requires: libbsd0

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

for F in CHANGES; do
    iconv -f iso-8859-1 -t utf-8 < "$F" > "${F}.new"
    touch -r "$F" "${F}.new"
    mv "${F}.new" "$F"
done

%build
export CFLAGS="$RPM_OPT_FLAGS -fPIE "
export LDFLAGS='-pie -Wl,-z,relro,-z,now,-z,noexecstack,-z,nodlopen'
%configure  --with-pidfile=/run/radvd/radvd.pid
make %{?_smp_mflags}

%install
make DESTDIR=%{buildroot} install

mkdir -p %{buildroot}%{_sysconfdir}/sysconfig
mkdir -p %{buildroot}/run/radvd
mkdir -p %{buildroot}%{_unitdir}

install -m 644 redhat/radvd.conf.empty %{buildroot}%{_sysconfdir}/radvd.conf
install -m 644 redhat/radvd.sysconfig %{buildroot}%{_sysconfdir}/sysconfig/radvd

install -d -m 755 %{buildroot}%{_tmpfilesdir}
install -p -m 644 %{SOURCE1} %{buildroot}%{_tmpfilesdir}/radvd.conf
install -p -m 644 redhat/radvd-tmpfs.conf %{buildroot}%{_tmpfilesdir}/radvd.conf
install -m 644 redhat/radvd.service %{buildroot}%{_unitdir}

%check
make check

%postun
%systemd_postun_with_restart radvd.service

%post
%systemd_post radvd.service

%preun
%systemd_preun radvd.service

# Static UID and GID defined by /usr/share/doc/setup-*/uidgid
%pre
getent group radvd >/dev/null || groupadd -r -g %RADVD_UID  radvd
getent passwd radvd >/dev/null || \
  useradd -r -u %RADVD_UID -g radvd -d / -s /sbin/nologin -c "radvd user" radvd
exit 0

%files
%doc CHANGES COPYRIGHT INTRO.html README TODO
%{_unitdir}/radvd.service
%config(noreplace) %{_sysconfdir}/radvd.conf
%config(noreplace) %{_sysconfdir}/sysconfig/radvd
%{_tmpfilesdir}/radvd.conf
%dir %attr(755,radvd,radvd) /run/radvd/
%doc radvd.conf.example
%{_mandir}/*/*
%{_sbindir}/radvd
%{_sbindir}/radvdump

