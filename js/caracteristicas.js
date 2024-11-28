document.addEventListener("DOMContentLoaded", () => {
    const mobileMenu = document.querySelector('.mobile-menu');
    const nav = document.querySelector('nav');

    mobileMenu.addEventListener('click', () => {
        nav.classList.toggle('active');
    });

    const links = document.querySelectorAll('header nav a');
    links.forEach(link => {
        link.addEventListener('click', (event) => {
            event.preventDefault();
            const href = link.getAttribute('href');

            if (href) {
                window.location.href = href;
            }
        });
    });
});
