Name: libmlocknice
Version: 0.1.2
Release: 1%{?dist}
Summary: Memory Locking library
Group: Development/Tools
License: GPLv2+
URL: http://www.gitorious.org/+maemo-tools-developers/maemo-tools/libmlocknice
Source: %{name}_%{version}.tar.gz
BuildRoot: {_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: autoconf, automake, libtool

%description
 Memory Locking library for maemo applications.
 
%prep
%setup -q -n %{name}

%build
./autogen.sh

%configure 

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
rm %{buildroot}/usr/lib/*.la

%clean
rm -rf %{buildroot}

%files
%defattr(644,root,root,-)
%{_libdir}/libmlocknice.so.0*
%doc README COPYING 

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%package -n %{name}-devel
Summary: Memory Locking library development files
Group: Development/Tools

%description -n %{name}-devel
 Memory Locking library development files
 
%files -n %{name}-devel
%defattr(644,root,root,-)
%{_libdir}/libmlocknice.a
%{_libdir}/libmlocknice.so
%{_includedir}/mlocknice.h
%doc README COPYING 

