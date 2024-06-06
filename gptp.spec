Name: gptp
Version: 1.0
Release: r0
Summary: Time Sensitive Networking stack Time Sync.
License: BSD-3-Clause & BSD-2-Clause & BSD-3-Clause-Clear
URL: https://www.codelinaro.org/
Source0: %{name}-%{version}.tar.gz

BuildRequires: autoconf automake libtool gcc-g++

%description
Time Sensitive Networking stack Time Sync.

%prep
%autosetup -n %{name}-%{version}

%build
%set_build_flags
make gptp libgptp libgptp_test

%install
mkdir -p %{buildroot}/%{_bindir}
mkdir -p %{buildroot}/%{_libdir}
mkdir -p %{buildroot}/%{_includedir}
mkdir -p %{buildroot}/%{_sysconfdir}
mkdir -p %{buildroot}/%{_unitdir}
install -m 755 daemons/gptp/linux/build/obj/qgptp %{buildroot}/%{_bindir}
install -m 755 examples/libgptp_test/libgptp_test %{buildroot}/%{_bindir}
install -m 755 lib/libgptp/*.so %{buildroot}/%{_libdir}
install -m 644 lib/libgptp/gptp_helper.h %{buildroot}/%{_includedir}
install -m 0644 daemons/gptp/gptp_cfg.ini %{buildroot}/%{_sysconfdir}
install -DpZm 0644 gptp.service %{buildroot}/%{_unitdir}

%post
systemctl enable gptp.service

%preun
%systemd_preun gptp.service

%postun
%systemd_postun_with_restart gptp.service

%files
%{_bindir}/qgptp
%{_bindir}/libgptp_test
%{_libdir}/libgptp.so
%{_includedir}/gptp_helper.h
%{_sysconfdir}/gptp_cfg.ini
%{_unitdir}/gptp.service

